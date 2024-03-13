#include <stdio.h>
#include "bootpack.h"

#define PORT_KEYDAT 0x0060

// 该程序是以INT 0x20~0x2f接收中断信号IRQ0~15而设定的
void init_pic(void){
	io_out8(PIC0_IMR, 0xff);/*IMR为8位寄存器，对应8路IRQ信号，禁止主PIC的全部中断*/
	io_out8(PIC1_IMR, 0xff);/*禁止从PIC的全部中断*/
	
	io_out8(PIC0_ICW1, 0x11);/* 边沿触发模式（edge trigger mode） */
	io_out8(PIC0_ICW2, 0x20);/* IRQ0-7由INT20-27接收 */
	io_out8(PIC0_ICW3, 1<<2);/* PIC1由IRQ2连接 */
	io_out8(PIC0_ICW4, 0x01);/* 无缓冲区模式 */
	
	io_out8(PIC1_ICW1, 0x11);/* 边沿触发模式（edge trigger mode） */
	io_out8(PIC1_ICW2, 0x28);/* IRQ8-15由INT28-2f接收 */
	io_out8(PIC1_ICW3,    2);/* PIC1由IRQ2连接 */
	io_out8(PIC1_ICW4, 0x01);/* 无缓冲区模式 */
	
	io_out8(PIC0_IMR, 0xfb); /* 11111011 对应IRQ2=0 主PIC0以外全部禁止 */
	io_out8(PIC1_IMR, 0xff); /* 11111111 禁止从PIC所有中断 */
	
	return;
}

void inthandler27(int *esp)
/* PIC0 的中断保护不完整 */
/* 在 Athlon64X2 机器等中，由于芯片组原因，此中断仅在初始化 PIC 时发生一次 */
/* 此中断处理函数对中断不执行任何操作 */
/* 为什么我什么都不做？
	→ 此中断是由 PIC 初始化期间的电气噪声引起的。不必认真对待任何事情。*/
{
	io_out8(PIC0_OCW2, 0x67); 
	return;
}
