#include "bootpack.h"

struct FIFO32 *keyfifo;
int keydata0;

//来自键盘的中断INT 0x21  编号为0x0060的设备就是键盘
void inthandler21(int *esp){
	int data;
	io_out8(PIC0_OCW2, 0x61);// PIC0_OCW2=0x0020 通知PIC:已经知道发生了IRQ1中断，将“0x60+IRQ号码”输出给OCW2就可以
	data=io_in8(PORT_KEYDAT);// 记录按下键盘中的数据
	
	fifo32_put(keyfifo, data+keydata0);
	
	return;
}

// #define KEYSTA_SEND_NOTREADY 	0x02
// #define PORT_KEYSTA 			0x0064/* 端口状态 */
//等待键盘控制电路准备完毕
void wait_KBC_sendready(void){
	/*如果键盘控制电路可以接受CPU指令了，CPU从设备号码0x0064处所读
		取的数据的倒数第二位（从低位开始数的第二位）应该是0*/
	for(;;){
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY)==0)
			break;
	}
	return;
}

// 初始化键盘端口
void init_keyboard(struct FIFO32 *fifo, int data0){
	/* 将FIFO缓冲区的信息保存到全局变量里 */
	keyfifo=fifo;
	keydata0=data0;
	/* 键盘控制器的初始化 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);//确认可否往命令端口发送指令信息
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);//向数据端口发送模式设定指令  KBC_MODE利用鼠标模式的模式号码是0x47
	return;
}
