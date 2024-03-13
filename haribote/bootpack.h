/* asmhead.nas */
struct BOOTINFO { /* 共是12字节0x0ff0-0x0fff */
	char cyls; /* 启动区读硬盘读到何处为止 */
	char leds; /* 启动时键盘LED的状态 */
	char vmode; /* 显卡模式为多少位彩色 */
	char reserve;
	short scrnx, scrny; /* 画面分辨率 */
	char *vram;
};
#define ADR_BOOTINFO	0x00000ff0
#define ADR_DISKIMG		0x00100000

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void load_tr(int tr);
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

/* graphic.c */
void init_palette(void);/* 初始化调色板 */
void set_palette(int start,int end,unsigned char* rgb);/* 设置调色板:将rgb数组中的数据写入到start-end调色板中 */
void boxfill8(unsigned char*vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1);/* 将坐标(x0,y0)和(x1,y1)围成的长方形内的像素颜色变成c */
void init_screen(char *vram, int x, int y);/* 绘制320*200内的像素屏幕框架 */
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);// 将font数组绘制到VRAM中以坐标(x, y)为起点处，字符颜色为c
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);//将字符串s绘制到VRAM中以坐标(x, y)为起点处，字符颜色为c
void init_mouse_cursor8(char *mouse, char bc);// 按照鼠标形状进行显示
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buff, int bxsize);//从buf读出pxsize*pysize个图形数据(bxsize指定读出哪些连续数据的)，并将该图形写到vram的(px0,py0)坐标处
#define COL8_000000		0 /*黑*/
#define COL8_FF0000		1 /*亮红*/
#define COL8_00FF00		2 /*亮绿*/
#define COL8_FFFF00		3 /*亮黄*/
#define COL8_0000FF		4 /*亮蓝*/
#define COL8_FF00FF		5 /*亮紫*/
#define COL8_00FFFF		6 /*浅亮蓝*/
#define COL8_FFFFFF		7 /*白*/
#define COL8_C6C6C6		8 /*亮灰*/
#define COL8_840000		9 /*暗红*/
#define COL8_008400		10/*暗绿*/
#define COL8_848400		11/*暗黄*/
#define COL8_000084		12/*暗蓝*/
#define COL8_840084		13/*暗紫*/
#define COL8_008484		14/*浅暗蓝*/
#define COL8_848484		15/*暗灰*/

/* dsctbl.c */
// GDT  global（segment）descriptor table
struct SEGMENT_DESCRIPTOR{
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};

// IDT  interrupt descriptor table
struct GATE_DESCRIPTOR{
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};

void init_gdtidt(void);//初始化GDT和IDT
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);// 设置GDT的相关信息
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);// 设置IDT的相关信息

#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_LDT 			0x0082
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e

/* int.c */
struct KEYBUF{
	unsigned char data[32];
	int	next_r, next_w, len;/*len是写入的字符个数 next_r下一个将读出的坐标 next_w下一个将写入的坐标*/
};
void init_pic(void);//初始化PIC
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

/* fifo.c */
// p下一个数据写入地址  q下一个数据读出地址  size已经写入的数据个数
// free缓冲区里没有数据的字节数
struct FIFO32 {
	int *buf;
	int p, q, size, free, flags;
	struct TASK *task;
};
void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);// 将fifo初始化
int fifo32_put(struct FIFO32 *fifo, int data);// 在fifo中插入数据data
int fifo32_get(struct FIFO32 *fifo);// 从fifo缓冲区中取出一个data数据
int fifo32_status(struct FIFO32 *fifo);// 获取当前fifo中已存数据的个数

/* mouse.c */
struct MOUSE_DEC{
	unsigned char buf[3], phase;//buf为鼠标的三元素 phase为鼠标的状态,分为0/1/2/3，0为等待状态，其他数字为等待写入第几个字节
	int x, y, btn;/* x,y存储鼠标的坐标，btn为鼠标的按键状态信息 */
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);// 与void init_keyboard(void)函数功能类似
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);/* 根据mdec->phase状态改写mdec内数据，并返回标志。0:未有数据写入，1:已有数据写入 */

/* keyboard.c */
#define PORT_KEYDAT 			0x0060/* 数据端口 */
#define PORT_KEYSTA 			0x0064/* 端口状态 */
#define PORT_KEYCMD 			0x0064/* 命令端口 */
#define KEYSTA_SEND_NOTREADY 	0x02
#define KEYCMD_WRITE_MODE 		0x60/* 模式设定指令 */
#define KBC_MODE 				0x47/* 利用鼠标模式的模式号码是0x47 */
void inthandler21(int *esp);
void wait_KBC_sendready(void);//等待键盘控制电路准备完毕
void init_keyboard(struct FIFO32 *fifo, int data0);// 初始化键盘端口

/* memory.c */
#define MEMMAN_FREES		4090
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO{
	unsigned int addr, size;// addr页的首地址  size页大小
};

struct MEMMAN{
	/*frees可用内存空间的个数   maxfrees最大的可用内存空间个数
	  losts分配空间失败的个数   lostsize分配失败的可用内存空间总和*/
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);//确定从start地址到end地址的范围内，能够使用的内存的末尾地址
void memman_init(struct MEMMAN *man);/* 内存信息初始化 */
unsigned int memman_total(struct MEMMAN *man);/* 统计整个内存中未使用的空间大小 */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);/* 在man内存中分配size大小的内存 */
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);/* 将addr开始的size大小内存释放到man中 */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);/* 在man中分配size的内存空间(向上舍入)，空间必须是4k的倍数 */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);/* 在man中释放addr开始的4k内存 */

/* sheet.c */
#define MAX_SHEETS 256

// buf记录图层形状 (vx0,vy0)图层定位 bxsize,bysize图层长宽 col_inv透明色色号 height图层高度 flags是否被使用
struct SHEET{/* 结构体中各个元素均为图层信息 */
	unsigned char *buf; 
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	struct SHTCTL *ctl;
	struct TASK *task;
};

// vramVRAM的地址 xsize,ysize画面大小 top最上面图层的高度 sheets0存放图层信息 sheets图层地址变量
// map存放画面上的点是哪个图层的像素
struct SHTCTL{
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
	
};
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);/* 在memman中创建SHTCTL结构体，并初始化 */
struct SHEET *sheet_alloc(struct SHTCTL *ctl);/* 在ctl中寻找一个未被使用的SHEET结构体，并返回该结构体的相关信息 */
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);/* 对sht中的元素进行初始化 */
void sheet_updown(struct SHEET *sht, int height);/* 将sht的图层顺序变成height，并更新排序 */
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);/* 刷新某一图层sht中的坐标(bx0, by0)与(bx1-1, by1-1)围成矩形范围内的图像 */
void sheet_slide(struct SHEET *sht, int vx0, int xy0);/* 仅上下左右移动图层,重新描绘移动前和移动后的地方 */
void sheet_free(struct SHEET *sht);/* 释放已使用图层 */

/* timer.c */
#define MAX_TIMER 500
struct TIMER{
	struct TIMER *next;// next为下一个即将超时的计时器地址
	unsigned int timeout;// timeout既定时刻  
	char flags, flags2;//flags记录各计时器状态
	struct FIFO32 *fifo;
	int data;
};

struct TIMERCTL{
	//count计时数 next记录下一个即将到达时刻 using记录活动计时器的个数
	unsigned int count, next, using;
	struct TIMER *t0;// 记录第一个计时器
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);// 计时器初始化
struct TIMER *timer_alloc(void);// 分配一个计时器
void timer_free(struct TIMER *timer);// 释放一个计时器
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);// 初始化一个计时器
void timer_settime(struct TIMER *timer, unsigned int timeout);// 设置一个计时器
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);
void inthandler20(int *esp);

/* mtask.c */
#define	MAX_TASKS 		1000	/*最大任务数量*/
#define TASK_GDT0 		3		/*定义从GDT的几号开始分配给TSS*/
#define MAX_TASKS_LV 	100		/*创建10个等级，每个等级中可有100个任务*/
#define MAX_TASKLEVELS 	10
struct FILEHANDLE{
	char *buf;
	int size;
	int pos;
};
struct TSS32 {
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;//与任务设置相关的信息
	/*EIP是CPU用来记录下一条需要执行的指令位于内存中哪个地址的寄存器*/
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};

struct TASK{
	/*flags 	0:未被使用状态	  1:休眠状态    2:唤醒状态*/
	int sel, flags;/* sel用来存放GDT的编号*/
	int level, priority;/*level任务等级 priority任务优先级*/
	struct FIFO32 fifo;
	struct TSS32 tss;
	struct SEGMENT_DESCRIPTOR ldt[2];
	struct CONSOLE *cons;
	int ds_base, cons_stack;
	struct FILEHANDLE *fhandle;
	int *fat;
	char *cmdline;
	char langmode, langbyte1;//langmode显示的语言格式，英语还是日语  langbyte1全角和半角标识位
};

struct TASKLEVEL{
	int running;/*正在运行的任务数量*/
	int now;/*记录当前正在运行的是哪个任务*/
	struct TASK *tasks[MAX_TASKS_LV];
};

struct TASKCTL{
	int now_lv; /*现在活动中的LEVEL */
	char lv_change; /*在下次任务切换时是否需要改变LEVEL */
	struct TASKLEVEL level[MAX_TASKLEVELS];
	struct TASK tasks0[MAX_TASKS];
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);
void task_idle(void);
struct TASK *task_now(void);

/*window.c*/
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);
void change_wtitle8(struct SHEET *sht, char act);

/*console.c*/
struct CONSOLE{
	struct SHEET *sht;
	int cur_x, cur_y, cur_c;
	struct TIMER *timer;
};
void console_task(struct SHEET *sheet, unsigned int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_newline(struct CONSOLE *cons);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal);
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_type(struct CONSOLE *cons, int *fat, char *cmdline);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
void cons_putstr0(struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0c(int *esp);
int *inthandler0d(int *esp);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1,int col);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);
void close_constask(struct TASK *task);
void close_console(struct SHEET *sht);

/*file.c*/

struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile2(int clustno, int *psize, int *fat);

/* tek.c */
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

/* bootpack.c */
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

