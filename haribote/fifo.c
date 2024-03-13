#include<stdio.h>
#include"bootpack.h"

#define FLAGS_OVERRUN		0x0001
/* FIFO结构体初始化 */
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task){
	fifo->buf = buf;	
	fifo->p = 0;		/* 下一个写入地址 */
	fifo->q = 0;		/* 下一个读出地址 */
	fifo->size = size;/* 缓冲区大小 */
	fifo->free = size;/* 可用缓冲区大小 */
	fifo->flags = 0;
	fifo->task = task;
	return;
}

/* 向fifo中写入data */
int fifo32_put(struct FIFO32 *fifo, int data){
	if(fifo->free==0){/* 没有可用的缓冲区，设置flags并返回-1 */
		fifo->flags|=FLAGS_OVERRUN;
		return -1;
	}
	fifo->free--;
	fifo->buf[fifo->p]=data;
	fifo->p++;
	if(fifo->p == fifo->size)fifo->p=0;
	if(fifo->task != 0){
		if(fifo->task->flags != 2)/*如果任务处于休眠状态*/
			task_run(fifo->task, -1, 0);/*将任务唤醒*/
	}
	return 0;
}

/* 从fifo缓冲区读出一个data数据 */
int fifo32_get(struct FIFO32 *fifo){
	int data;
	if(fifo->free==fifo->size){/* 缓冲区内没有数据，直接返回-1 */
		return -1;
	}
	data = fifo->buf[fifo->q];
	fifo->q++;
	fifo->free++;
	if(fifo->q==fifo->size)fifo->q=0;
	return data;
}

/* 报告缓冲区内已存数据的个数 */
int fifo32_status(struct FIFO32 *fifo){
	return fifo->size - fifo->free;
}