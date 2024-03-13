#include"bootpack.h"

/* 在memman中创建SHTCTL结构体，并初始化 */
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize){
	int i;
	struct SHTCTL *ctl;
	ctl=(struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	
	ctl->map=(unsigned char*)memman_alloc_4k(memman, xsize*ysize);
	if(ctl->map==0){
		memman_free_4k(memman, (int)ctl, sizeof(struct SHTCTL));
		goto err;
	}
	
	ctl->vram=vram;
	ctl->xsize=xsize;
	ctl->ysize=ysize;
	ctl->top=-1;
	for(i=0;i<MAX_SHEETS;i++){
		ctl->sheets0[i].flags=0;/* 标记为未使用 */
		ctl->sheets0[i].ctl=ctl;
	}
err:
	return ctl;
}

/* 在ctl中寻找一个未被使用的SHEET结构体，并返回该结构体的相关信息 */
#define SHEET_USE	1
struct SHEET *sheet_alloc(struct SHTCTL *ctl){
	struct SHEET *sht;
	int i;
	for(i=0;i<MAX_SHEETS;i++){
		if(ctl->sheets0[i].flags==0){
			sht=&ctl->sheets0[i];
			sht->flags=SHEET_USE;/* 标记成正在使用 */
			sht->height=-1;//隐藏
			sht->task = 0;
			return sht;
		}
	}
	return 0;/* 所有的SHEET都处于正在使用状态*/
}

/* 对sht中的元素进行初始化 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
	sht->buf=buf;
	sht->bxsize=xsize;
	sht->bysize=ysize;
	sht->col_inv=col_inv;
	return;
}

/* 将sht的图层顺序变成height，并更新排序 */
void sheet_updown(struct SHEET *sht, int height){
	struct SHTCTL *ctl=sht->ctl;
	int h, old=sht->height;
	/* 如果指定的高度过高或过低，则进行修正 */
	if(height > ctl->top+1){
		height=ctl->top+1;
	}
	if(height<-1)height=-1;
	sht->height=height;/* 设定高度 */
	
	/* 下面主要是进行sheets[ ]的重新排列 */
	if(old>height){/* 比以前低 */
		if(height>=0){
			/* 把中间的往上提 */
			for(h=old;h>height;h--){
				ctl->sheets[h]=ctl->sheets[h-1];
				ctl->sheets[h]->height=h;
			}
			ctl->sheets[height]=sht;
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, height+1);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, height+1, old);
		}
		else {/* 隐藏 */
			if(ctl->top>old){
				/* 把上面的降下来 */
				for(h=old;h<ctl->top;h++){
					ctl->sheets[h]=ctl->sheets[h+1];
					ctl->sheets[h]->height=h;
				}
			}
			ctl->top--;/* 正在显示的图层减少了一个，故最上面的高度也减少 */
			sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, 0);
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, 0, old-1);
		}
	}
	else if(old<height){/* 比以前高 */
		if(old>=0){/* 中间的图层往下降一层 */
			for(h=old;h<height;h++){
				ctl->sheets[h]=ctl->sheets[h+1];
				ctl->sheets[h]->height=h;
			}
			ctl->sheets[height]=sht;
		}
		else {/* 从隐藏状态变为显示状态 */
			for(h=ctl->top;h>=height;h--){
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++; /* 由于已显示的图层增加了1个，所以最上面的图层高度增加 */
		}
		sheet_refreshmap(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, height);
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize, height, height);
	}
	return;
}

/* 将透明以外的所有像素都复制到VRAM中
   刷新某一图层sht中的坐标(bx0, by0)与(bx1-1, by1-1)围成矩形范围内的图像，仅刷新当前图层 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1){
	if(sht->height>=0)/* 如果正在显示，则按新图层的信息刷新画面*/
		sheet_refreshsub(sht->ctl, sht->vx0+bx0, sht->vy0+by0, sht->vx0+bx1, sht->vy0+by1, sht->height, sht->height);
	return;
}

/* 仅上下左右移动图层,重新描绘移动前和移动后的地方 */
void sheet_slide(struct SHEET *sht, int vx0, int vy0){
	int old_vx0=sht->vx0, old_vy0=sht->vy0;
	struct SHTCTL *ctl = sht->ctl;
	sht->vx0=vx0;
	sht->vy0=vy0;
	if(sht->height>=0){/* 如果正在显示，则按新图层的信息刷新画面 */
		sheet_refreshmap(ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap(ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0+sht->bxsize, old_vy0+sht->bysize, 0, sht->height-1);
		sheet_refreshsub(ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize, sht->height, sht->height);
	} 
	
	return;
}

/* 释放已使用图层 */
void sheet_free(struct SHEET *sht){
	if(sht->height>=0)
		sheet_updown(sht, -1);
	sht->flags=0;
	return;
}

/* 更新ctl的各图层(vx0,vy0)(vx1,vy1)范围内的像素 */
// void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1){
	// int h, bx, by, vx, vy;
	// unsigned char *buf, c, *vram=ctl->vram;
	// struct SHEET *sht;
	// for(h=0;h<=ctl->top;h++){
		// sht=ctl->sheets[h];
		// buf=sht->buf;
		// for(by=0;by<sht->bysize;by++){
			// vy=sht->vy0+by;
			// for(bx=0;bx<sht->bxsize;bx++){
				// vx=sht->vx0+bx;
				// if(vx0<=vx && vx<vx1 && vy0<=vy && vy<vy1){
					// c=buf[by*sht->bxsize+bx];
					// if(c!=sht->col_inv)
						// vram[vy*ctl->xsize+vx]=c;
				// }
			// }
		// }
	// }
	// return;
// }

/*刷新图层数组ctl中坐标(vx0, vy0)与(vx1-1, vy1-1)围成矩形内的、高度范围h0~h1的图层图像*/
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1){
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, bx2, sid4, i, i1, *p, *q, *r;
	unsigned char *buf, *vram=ctl->vram,  *map = ctl->map, sid;
	struct SHEET *sht;
	/* 如果refresh的范围超出了画面则修正*/
	if(vx0<0)vx0=0;
	if(vy0<0)vy0=0;
	if(vx1>ctl->xsize)vx1=ctl->xsize;
	if(vy1>ctl->ysize)vy1=ctl->ysize;
	
	for(h=h0;h<=h1;h++){
		sht=ctl->sheets[h];
		sid=sht-ctl->sheets0;
		buf=sht->buf;
		/* 使用vx0～vy1，对bx0～by1进行倒推
		  计算vx0在当前图层下的坐标(当前图层的(sht->vx0, sht->vy0)为原点)
		  bx0、by0若小于0则超出当前图层的图像区间，则置为0；
          bx1、by1若大于当前图层的图像大小(超出部分没有图像，没有必要刷新)，则置为图像的大小；*/
		bx0=vx0-sht->vx0;
		by0=vy0-sht->vy0;
		bx1=vx1-sht->vx0;
		by1=vy1-sht->vy0;
		
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		
		if((sht->vx0&3)==0){//图层的x坐标是4的倍数
			i = (bx0+3)/4;
			i1= bx1/4;
			i1= i1-i;
			sid4 = sid|sid<<8|sid<<16|sid<<24;
			for(by = by0; by < by1; by++){
				vy = sht->vy0+by;
				for (bx = bx0; bx < bx1 && (bx&3) != 0; bx++){//先写入前面非4倍数的地址数据
					vx = sht->vx0+bx;
					if(map[vy*ctl->xsize+vx] == sid)
						vram[vy*ctl->xsize+vx] = buf[by*sht->bxsize+bx];
				}
				vx = sht->vx0+bx;
				p = (int*)&map[vy*ctl->xsize+vx];
				q = (int*)&vram[vy*ctl->xsize+vx];
				r = (int*)&buf[by*sht->bxsize+bx];
				for(i = 0; i < i1; i++){//中间4的倍数部分
					if(p[i] == sid4)q[i] = r[i];
					else{
						bx2 = bx+i*4;
						vx = sht->vx0+bx2;
						if(map[vy*ctl->xsize+vx+0] == sid)
							vram[vy*ctl->xsize+vx+0] = buf[by*sht->bxsize+bx+0];
						if(map[vy*ctl->xsize+vx+1] == sid)
							vram[vy*ctl->xsize+vx+1] = buf[by*sht->bxsize+bx+1];
						if(map[vy*ctl->xsize+vx+2] == sid)
							vram[vy*ctl->xsize+vx+2] = buf[by*sht->bxsize+bx+2];
						if(map[vy*ctl->xsize+vx+3] == sid)
							vram[vy*ctl->xsize+vx+3] = buf[by*sht->bxsize+bx+3];
					}
				}
				for(bx += i1; bx < bx1; bx++){//最后写入后面非4倍数的地址数据
					vx = sht->vx0+bx;
					if(map[vy*ctl->xsize+vx] == sid)
						vram[vy*ctl->xsize+vx] = buf[by*sht->bxsize+bx];
				}
			}
		}else{
			for (by = by0; by < by1; by++) {
				vy = sht->vy0+by;
				for (bx = bx0; bx < bx1; bx++) {
					vx = sht->vx0+bx;
					if (map[vy*ctl->xsize+vx] == sid) {
						vram[vy*ctl->xsize+vx] = buf[by*sht->bxsize+bx];
					}
				}
			}
		}
	}
	return;
}
// 在ctl中，更新map中高度h0及以上的图层中坐标(vx0, vy0)和(vx1, vy1)围成矩形范围的图层号码
void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0){
	int h, bx, by, vx, vy, bx0, by0, bx1, by1, sid4, *p;
	unsigned char *buf, sid, *map=ctl->map;
	struct SHEET *sht;
	if(vx0<0)vx0=0;
	if(vy0<0)vy0=0;
	if(vx1>ctl->xsize)vx1=ctl->xsize;
	if(vy1>ctl->ysize)vy1=ctl->ysize;
	
	for(h=h0;h<=ctl->top;h++){
		sht=ctl->sheets[h];
		sid=sht-ctl->sheets0;/* 将进行了减法计算的地址作为图层号码使用 */
		buf=sht->buf;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		if(sht->col_inv == -1){// 无透明色图层
			if((sht->vx0&3) == 0 && (bx0&3) == 0 && (bx1&3) == 0){//4字节
				bx1 = (bx1-bx0)/4;//MOV次数
				sid4 = sid | sid << 8 | sid << 16 | sid << 24;//每次写入4字节
				for (by = by0; by < by1; by++) {
					vy = sht->vy0 + by;
					vx = sht->vx0 + bx0;
					p = (int *) &map[vy * ctl->xsize + vx];
					for(bx = 0; bx < bx1; bx++)
						p[bx] = sid4;
				}	
			}else{//1字节
				for (by = by0; by < by1; by++) {
					vy = sht->vy0 + by;
					for (bx = bx0; bx < bx1; bx++) {
						vx = sht->vx0 + bx;// 不是透明色表明该位置有图形像素，更新对应map中的数据
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		}else{// 有透明色的图层
			for (by = by0; by < by1; by++) {
				vy = sht->vy0 + by;
				for (bx = bx0; bx < bx1; bx++) {
					vx = sht->vx0 + bx;// 不是透明色表明该位置有图形像素，更新对应map中的数据
					if (buf[by * sht->bxsize + bx] != sht->col_inv) {
						map[vy * ctl->xsize + vx] = sid;
					}
				}
			}
		}
	}
	return;
}
