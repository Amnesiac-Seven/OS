#include "bootpack.h"

//将磁盘映像文件中的数据转换成FAT（文件分配表file allocation table）
void file_readfat(int *fat, unsigned char *img){
	int i, j = 0;
	for(i = 0; i < 2880; i+=2){
		fat[i+0] = (img[j+0] 	| img[j+1]<<8)&0xfff;
		fat[i+1] = (img[j+2]<<4 | img[j+1]>>4)&0xfff;
		j += 3;
	}
	return;
}
//加载文件
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img){
	int i;
	for(;;){
		if(size <= 512){//文件大小<512字节
			for(i = 0; i < size; i++)
				buf[i] = img[clustno*512+i];
			break;
		}
		for(i = 0; i < 512; i++)//仅读前512字节
			buf[i] = img[clustno*512+i];
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}
//遍历finfo，返回与name相同字符串的文件，否则返回0
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
	int i, j;
	char s[12];
	for (j = 0; j < 11; j++) {
		s[j] = ' ';
	}
	j = 0;
	for (i = 0; name[i] != 0; i++) {
		if (j >= 11) { return 0; /* 没找到 */ }
		if (name[i] == '.' && j <= 8) {
			j = 8;
		} else {
			s[j] = name[i];
			if ('a' <= s[j] && s[j] <= 'z') {/* 小写字母变大写字母 */
				s[j] -= 0x20;
			} 
			j++;
		}
	}
	for (i = 0; i < max; ) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if ((finfo[i].type & 0x18) == 0) {
			for (j = 0; j < 11; j++) {
				if (finfo[i].name[j] != s[j]) {
					goto next;
				}
			}
			return finfo + i; /* 返回文件所在地址 */
		}
next:
		i++;
	}
	return 0; /* 没找到 */
}

char *file_loadfile2(int clustno, int *psize, int *fat){
	int size = *psize, size2;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;
	char *buf, *buf2;
	buf = (char*)memman_alloc_4k(memman, size);
	file_loadfile(clustno, size, buf, fat, (char*)(ADR_DISKIMG+0x003e00));
	if(size >= 17){//文件大小超17字节，可能位tek格式的文件
		size2 = tek_getsize(buf);
		if(size2 > 0){/*使用tek格式压缩的文件*/
			buf2 = (char*)memman_alloc_4k(memman, size2);
			tek_decomp(buf, buf2, size2);//解压
			memman_free_4k(memman, (int)buf, size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}
