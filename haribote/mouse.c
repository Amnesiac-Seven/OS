#include"bootpack.h"
#define KEYCMD_SENDTO_MOUSE 0xd4
#define MOUSECMD_ENABLE 0xf4
	
struct FIFO32 *mousefifo;
int mousedata0;
	
/* 来自鼠标的中断INT 0x2c */
void inthandler2c(int *esp)
{
	int data;
	/* 从PIC的下标也是从0开始，下标0对应IRQ8，所以下标4对应IRQ12 */
	io_out8(PIC1_OCW2, 0x64);/* 通知PIC1 IRQ-12的受理已经完成 */
	io_out8(PIC0_OCW2, 0x62);/* 通知PIC0 IRQ-02的受理已经完成 */
	data=io_in8(PORT_KEYDAT);// 记录按下键盘中的数据
	
	fifo32_put(mousefifo, data+mousedata0);
	
	return;
}

// 与void init_keyboard(void)
/* mouse_dbuf[0]表示鼠标的动作，高4位取值范围是0-3，低4位取值范围8-F，如果出现其他值也说明鼠标发生错误
   高4位是表示对移动有反应：高4位为1说明左右移动，高4位为2说明上下移动
   如果低4位为9说明鼠标左键按下，低4位为a说明右键被按下，双键同时按下为b，滚轮按下为c
   mouse_dbuf[1]与鼠标的左右移动有关系，mouse_dbuf[2]则与鼠标的上下移动有关系 */
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec){
	mousefifo=fifo;
	mousedata0=data0;
	/* 激活鼠标，如果往键盘控制电路发送指令0xd4，下一个数据就会自动发送给鼠标*/
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);//确认可否往命令端口发送指令信息
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);//向数据端口发送模式设定指令
	mdec->phase=0;/* 进入等待鼠标的0xfa的状态   enable_mouse()函数设置初始值MOUSECMD_ENABLE=0xfa */
	return; /* 顺利的话,键盘控制其会返送回ACK(0xfa)*/
}

/* 根据mdec->phase状态改写mdec内数据，并返回标志。0:未有数据写入，1:已有数据写入 */
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if(mdec->phase == 0){
		if(dat==0xfa)mdec->phase = 1;/* 初始化后等待鼠标的0xfa状态，把最初读到的0xfa舍弃掉 */
		return 0;
	}
	if(mdec->phase == 1){
		if((dat & 0xc8) == 0x08){// 高4位取值范围是0-3，低4位取值范围8-F 
			mdec->buf[0]=dat;
			mdec->phase=2;
		}
		
		return 0;
	}
	if(mdec->phase == 2){
		mdec->buf[1]=dat;
		mdec->phase=3;
		return 0;
	}
	if(mdec->phase == 3){
		mdec->buf[2]=dat;
		mdec->phase=1;
		mdec->btn=mdec->buf[0]&0x07;// 状态
		mdec->x=mdec->buf[1];// 左右移动
		mdec->y=mdec->buf[2];// 上下移动
		
		if((mdec->buf[0] & 0x10)!=0)mdec->x |= 0xffffff00;
		if((mdec->buf[0] & 0x20)!=0)mdec->y |= 0xffffff00;
		mdec->y = -mdec->y;// 鼠标与屏幕的y方向正好相反， 为了配合画面方向，就对y符号进行了取反操作
		return 1;
	}
	return -1;
}
