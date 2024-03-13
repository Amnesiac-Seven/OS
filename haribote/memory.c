#include"bootpack.h"

#define EFLAGS_AC_BIT 		0x00040000
#define CR0_CACHE_DISABLE 	0x60000000

//确定从start地址到end地址的范围内，能够使用的内存的末尾地址
unsigned int memtest(unsigned int start, unsigned int end){
	char flg486=0;
	unsigned int eflg, cr0, i;
	
	/* 确认CPU是386还是486以上的。如果是486以上，EFLAGS寄存器的第18位应该是所谓的AC标志位；
		如果CPU是386，那么就没有这个标志位，第18位一直是0。 */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	
	if(eflg&EFLAGS_AC_BIT){/* 如果是386，即使设定AC=1，AC的值还会自动回到0 */
		flg486=1;
	}
	
	eflg&= ~EFLAGS_AC_BIT;/* 将AC标志位重置为0 */
	io_store_eflags(eflg);
	
	if(flg486){
		cr0=load_cr0();
		cr0|=CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}
	
	i=memtest_sub(start, end);
	
	if(flg486){
		cr0=load_cr0();
		cr0&= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}
	return i;
}

/* 内存信息初始化 */
void memman_init(struct MEMMAN *man){
	man->frees=0;		/* 可用信息数目 */
	man->maxfrees=0;	/* 用于观察可用状况：frees的最大值 */
	man->lostsize=0;	/* 释放失败的内存的大小总和 */
	man->losts=0;		/* 释放失败次数 */
	return;
}

/* 统计整个内存中未使用的空间大小 */
unsigned int memman_total(struct MEMMAN *man){
	unsigned int i, t=0;
	for(i=0;i<man->frees;i++){
		t+=man->free[i].size;
	}
	return t;
}

/* 在man分配内存：1.检索能放下size空间大小的内存，2.更新该块内存的信息，
   3.检查该内存是否还有剩余内存，没有则更新数组信息 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	unsigned int i, a;
	for(i=0;i<man->frees;i++){
		if(man->free[i].size>size){/* 找到了足够大的内存 */
			a=man->free[i].addr;
			man->free[i].addr+=size;
			man->free[i].size-=size;
			if(man->free[i].size==0){/* 如果free[i]变成了0，表明该区间没有可用内存，就减掉一条可用信息 */
				man->frees--;
				for(;i<man->frees;i++){/* 代入结构体,后续空间前移 */
					man->free[i] = man->free[i + 1]; 
				}
			}
			return a;
		}
	}
	return 0;/* 没有可用空间 */
}

/* 释放内存空间，有4种情况
	1.可以与前面相邻的内存空间进行合并(需要考虑是否前面存在内存空间)；
	2.可以与后面相邻空间进行合并(man->free数组需要移动)；
	3.不能与前后相邻内存空间进行合并(需要扩展man->free数组存放待释放的内存)；
	4.待释放的内存无法被释放*/
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/* 为便于归纳内存，将free[]按照addr的顺序排列 */
	/* 所以，先决定应该放在哪里 */
	for (i = 0; i < man->frees; i++) {/* 表明addr在man[i].free之前，即确定了释放空间位置 */
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* 前面有可用内存，即不是释放到整个内存空间头部 */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* 可以与前面的可用内存归纳到一起 */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* 紧后也有可用内存 */
				if (addr + size == man->free[i].addr) {
					/* 也可以与后面的可用内存归纳到一起 */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]删除 */
					/* free[i]变成0后归纳到前面去 */
					man->frees--;
					/* 由于下标i的内存与i-1内存进行了合并，所以后续空间前移 */
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; 
					}
				}
			}
			return 0; /* 成功完成 */
		}
	}
	/* 不能与前面的可用空间归纳到一起 */
	if (i < man->frees) {
		/* 后面还有，正好与下标为i的内存连接 */
		if (addr + size == man->free[i].addr) {
			/* 可以与后面的内容归纳到一起 */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功完成 */
		}
	}
	/* 既不能与前面归纳到一起，也不能与后面归纳到一起 */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]之后的，向后移动，腾出一个可用空间 */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* 更新最大值 */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功完成 */
	}
	/* 不能往后移动 */
	man->losts++;
	man->lostsize += size;
	return -1; /* 失败 */
}

/* 在man中分配size的内存空间(向上舍入)，空间必须是4k的倍数 */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
	unsigned int a;
	size=(size+0xfff)&0xfffff000;// 计算存放size内存的最小4k倍数
	a=memman_alloc(man,size);
	return a;
}

/* 在man中释放addr开始的4k内存 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
	int i;
	size=(size+0xfff)&0xfffff000;
	i=memman_free(man, addr, size);
	return i;
}

