/* bootpack */

#include "bootpack.h"
#include <stdio.h>

#define KEYCMD_LED 0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
void close_console(struct SHEET *sht);
void close_constask(struct TASK *task);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct SHTCTL *shtctl;
	char s[40];
	struct FIFO32 fifo, keycmd;
	int fifobuf[128], keycmd_buf[32];
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	unsigned char *buf_back, buf_mouse[256];
	struct SHEET *sht_back, *sht_mouse;
	struct TASK *task_a, *task;
	static char keytable0[0x80] = {
		  0,   0, '1', '2', '3', '4', '5', '6',  '7', '8', '9', '0',  '-', '=', 0x08,   0,/*TAB*/
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',  'O', 'P', '[', ']', 0x0a,   0,  'A', 'S',/*0:CAPSLOCK*/
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'',   0,   0,   0,  'Z', 'X',  'C', 'V',
		'B', 'N', 'M', ',', '.', '/',   0, '*',    0, ' ',   0,   0,    0,   0,    0,   0,
		  0,   0,   0,   0,   0,   0,   0, '7',  '8', '9', '-', '4',  '5', '6',  '+', '1',
		'2', '3', '0', '.',   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,    0,   0,
		  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,    0,   0,
		  0,   0,   0,0x5c,   0,   0,   0,   0,    0,   0,   0,   0,    0,0x5c,    0,   0
	};
	static char keytable1[0x80] = {
		  0,   0, '!',  '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0x08,   0,
		'Q', 'W', 'E',  'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',0x0a,   0,  'A', 'S',
		'D', 'F', 'G',  'H', 'J', 'K', 'L', ':','\"',   0,   0,   0, 'Z', 'X',  'C', 'V',
		'B', 'N', 'M',  '<', '>', '?',   0, '*',   0, ' ',   0,   0,   0,   0,    0,   0,
		  0,   0,   0,    0,   0,   0,   0, '7', '8', '9', '-', '4', '5', '6',  '+', '1',
		'2', '3', '0',  '.',   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,
		  0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,
		  0,   0,   0,  '_',   0,   0,   0,   0,   0,   0,   0,   0,   0, '|',    0,   0
	};
	//key_leds包含ScrollLock、NumLock和CapsLock状态
	int key_shift = 0,  key_leds = (binfo->leds>>4)&7, keycmd_wait = -1;
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;// mmx, mmy鼠标移动前的坐标  x, y是移动后的坐标
	struct SHEET *sht = 0, *key_win, *sht2;//key_win存放的是当前最上面窗口的图层地址
	int *fat;
	unsigned char *nihongo;
	struct FILEINFO *finfo;
	extern char hankaku[4096];

	init_gdtidt();
	init_pic();
	io_sti(); /* IDT/PIC */
	/* 初始化内存 */
	fifo32_init(&fifo, 128, fifobuf, 0);
	*((int *) 0x0fec) = (int) &fifo;
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); /* 开放IRQ 0/1/2  linuxP158*/
	io_out8(PIC1_IMR, 0xef); /* 开放IRQ 12 */
	fifo32_init(&keycmd, 32, keycmd_buf, 0);

	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	
	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	task_a = task_init(memman);
	fifo.task = task_a;
	task_run(task_a, 1, 2);
	*((int *) 0x0fe4) = (int) shtctl;
	task_a->langmode = 0;
	
	/* sht_back */
	sht_back  = sheet_alloc(shtctl);/* 分配图层 */
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);/* 分配内存 */
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 设置图层缓冲区 */
	init_screen (buf_back, binfo->scrnx, binfo->scrny);/* 绘制背景 */
	
	/* sht_cons */
	key_win = open_console(shtctl, memtotal);
		
	/* sht_mouse */
	sht_mouse = sheet_alloc(shtctl);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	init_mouse_cursor8(buf_mouse, 99);
	mx = (binfo->scrnx - 16) / 2; /* 屏幕中心坐标 */
	my = (binfo->scrny - 28 - 16) / 2;	
	
	/* 调整背景、鼠标和窗口的显示位置 */
	sheet_slide(sht_back,   0,  0);
	sheet_slide(key_win,   32,  4);
	sheet_slide(sht_mouse, mx, my);
	
	/* 调整背景、鼠标和窗口的图层高度 */
	sheet_updown(sht_back,  0);
	sheet_updown(key_win,   1);
	sheet_updown(sht_mouse, 2);
	keywin_on(key_win);
	
	/*为了避免和键盘当前状态冲突，在一开始先进行设置*/
	fifo32_put(&keycmd, KEYCMD_LED);
	fifo32_put(&keycmd, key_leds);
	
	/*载入nihongo.fnt */
	fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	finfo = file_search("nihongo.fnt", (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	if (finfo != 0) {
		i = finfo->size;
		nihongo = file_loadfile2(finfo->clustno, &i, fat);
	} else {
		nihongo = (unsigned char *)memman_alloc_4k(memman, 16*256+32* 94 * 47);
		for (i = 0; i < 16 * 256; i++) {
			nihongo[i] = hankaku[i]; /*没有字库，半角部分直接复制英文字库*/
		}
		for (i = 16 * 256; i < 16 * 256 + 32 *  94 * 47; i++) {
			nihongo[i] = 0xff; /*没有字库，全角部分以0xff填充*/
		}
	}
	*((int *) 0x0fe8) = (int) nihongo;
	memman_free_4k(memman, (int) fat, 4 * 2880);
	
	for (;;) {
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			/*如果存在向键盘控制器发送的数据，则发送它 */
			keycmd_wait = fifo32_get(&keycmd);//读取状态寄存器
			wait_KBC_sendready();
			io_out8(PORT_KEYDAT, keycmd_wait);//向数据输出（0060）写入要发送的1个字节数据
		}
		io_cli();
		if (fifo32_status(&fifo) == 0) {
			/* FIFO为空，当存在搁置的绘图操作时立即执行*/
			if(new_mx >= 0){
				io_sti();
				sheet_slide(sht_mouse, new_mx, new_my);
				new_mx = -1;
			}else if(new_wx != 0x7fffffff){
				io_sti();
				sheet_slide(sht, new_wx, new_wy);
				new_wx = 0x7fffffff;
			}else{
				task_sleep(task_a);
				io_sti();
			}
		} else {
			i = fifo32_get(&fifo);
			io_sti();
			if(key_win->flags == 0){/*输入窗口被关闭*/
				key_win = shtctl->sheets[shtctl->top-1];
				keywin_on(key_win);
			}
			if(key_win != 0 && key_win->flags == 0){/*窗口被关闭,则将下一个窗口设为当前窗口*/
				if(shtctl->top == 1)key_win = 0;
				else{
					key_win = shtctl->sheets[shtctl->top-1];
					keywin_on(key_win);
				}
			}
			if (256 <= i && i <= 511) {/* 键盘数据 */
				if(i < 256 + 0x80){/*将按键编码转换为字符编码*/
					if(key_shift == 0)
						s[0] = keytable0[i-256];
					else
						s[0] = keytable1[i-256];
				}else{
					s[0] = 0;
				}
				if('A' <= s[0] && s[0] <= 'Z'){/*当输入字符为英文字母时*/
					if (((key_leds & 4) == 0 && key_shift == 0) ||
							((key_leds & 4) != 0 && key_shift != 0)){/*没有按下CapsLock和shift键 或者同时按下CapsLock和shift键*/
						s[0]+=0x20;/*将大写字母转换为小写字母*/
					}
				}
				if(s[0] != 0 && key_win != 0){/*一般字符、退格键、回车键*/
					fifo32_put(&key_win->task->fifo, s[0]+256);
				}
				if(i == 256 + 0x0f && key_win != 0){/* Tab键*/
					keywin_off(key_win);
					j = key_win->height-1;
					if(j == 0)j = shtctl->top-1;
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if(i == 256+0x2a){/*左Shift ON */
					key_shift |= 1;
				}
				if(i == 256+0x36){/*右Shift ON */
					key_shift |= 2;
				}
				if (i == 256+0xaa) { /*左Shift OFF */
					key_shift &= ~1;
				}
				if (i == 256+0xb6) { /*右Shift OFF */
					key_shift &= ~2;
				}
				if (i == 256 + 0x3a) { /* CapsLock */
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) { /* NumLock */
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) { /* ScrollLock */
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if(i == 256 + 0x3b && key_shift != 0 && key_win != 0){/* Shift+F1 */
					task = key_win->task;
					if(task != 0 && task->tss.ss0 != 0){
						cons_putstr0(task->cons, "\nBreak(key):\n");
						io_cli();/*不能在改变寄存器值时切换到其他任务*/
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;/*修改执行下一个指令的寄存器，也就是指向下一条指令*/
						io_sti();
						task_run(task, -1, 0);/*为了确实执行结束处理，如果处于休眠状态则唤醒*/
					}
				}
				if(i == 256 + 0x3c && key_shift != 0){/* Shift+F2 开启新命令窗口 */
					if(key_win != 0)
						keywin_off(key_win);
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					/*自动将输入焦点切换到新打开的命令行窗口*/
					keywin_on(key_win);
				}
				if(i == 256 + 0x57)/*按下F11，将最下面的图层放在最上面*/
					sheet_updown(shtctl->sheets[1], shtctl->top-1);
				if (i == 256 + 0xfa) { /*键盘成功接收到数据*/
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) { /*键盘没有成功接收到数据*/
					wait_KBC_sendready();
					io_out8(PORT_KEYDAT, keycmd_wait);//向数据输出（0060）写入要发送的1个字节数据
				}
			} else if (512 <= i && i <= 767) {/* 鼠标数据 */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* 更新鼠标的位置 */
					mx += mdec.x;
					my += mdec.y;
					new_mx = mx;
					new_my = my;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					
					sheet_slide(sht_mouse, mx, my);
					if((mdec.btn&0x01) != 0){// 如果左键按下，则移动窗口
						if(mmx < 0){
							/*如果处于通常模式，窗口不移动*/
							/*按照从上到下的顺序寻找鼠标所指向的图层*/
							for(j = shtctl->top-1; j > 0; j--){
								sht = shtctl->sheets[j];
								y = my - sht->vy0;
								x = mx - sht->vx0;
								if(0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize){
									if(sht->buf[y*sht->bxsize+x] != sht->col_inv){
										sheet_updown(sht, shtctl->top-1);
										if(key_win != sht){
											keywin_off(key_win);/*调整原key_win的图层光标为OFF*/
											key_win = sht;
											keywin_on(key_win);/*调整刚被选中的图层光标为ON*/
										}
										if(3 <= x && x < sht->bxsize-3 && 3<= y && y < 21){//判断是否在窗口的标题栏
											mmx = mx;
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if(sht->bxsize-21 <= x && x < sht->bxsize-5 && 5<= y && y < 19){//点击"x"关闭窗口
											if((sht->flags & 0x10) != 0){/*该窗口不是应用程序窗口*/
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli();
												task->tss.eax = (int)&(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti();
											}else{//是命令行窗口
												task = sht->task;
												sheet_updown(sht, -1);/*暂且隐藏该图层*/
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												fifo32_put(&task->fifo, 4);
												io_sti();
											}
											task_run(task, -1, 0);/*为了确实执行结束处理，如果处于休眠状态则唤醒*/
										}
										break;
									}
								}
							}
						}else{/*窗口移动模式，更新图层的位置*/
							y = my - mmy;
							x = mx - mmx;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;
						}
					}else{/*没有按下左键*/
						mmx = -1;/*返回通常模式*/
						if(new_wx != 0x7fffffff){
							sheet_slide(sht, new_wx, new_wy);/*刷新图层位置*/
							new_wx = 0x7fffffff;
						}
					}
				}
			}else if(768 <= i && i <= 1023){/*命令行窗口关闭处理*/
				close_console(shtctl->sheets0 + (i - 768));
			}else if(1024 <= i && i <= 2023){/*命令行任务关闭处理*/
				close_constask(taskctl->tasks0 + (i - 1024));
			}else if(2024 <= i && i <= 2279){/*只关闭命令行窗口*/
				sht2 = shtctl->sheets0+(i-2024);
				memman_free_4k(memman, (int)sht2->buf, 256*165);
				sheet_free(sht2);
			}
		}
	}
}
//控制窗口标题栏的颜色和task_a窗口的光标
void keywin_off(struct SHEET *key_win){
	change_wtitle8(key_win, 0);
	if((key_win->flags&0x20) != 0){
		fifo32_put(&key_win->task->fifo, 3); /*命令行窗口光标OFF */
	}
	return;
}

void keywin_on(struct SHEET *key_win){
	change_wtitle8(key_win, 1);
	if((key_win->flags&0x20) != 0){
		fifo32_put(&key_win->task->fifo, 2); /*命令行窗口光标ON */
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *) (task->tss.esp + 4)) = (int) sht;
	*((int *) (task->tss.esp + 8)) = memtotal;
	task_run(task, 2, 2); /* level=2, priority=2 */
	fifo32_init(&task->fifo, 128, cons_fifo, task);
	return task;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct	SHEET *sht = sheet_alloc(shtctl);/* 分配图层 */
	unsigned char *buf = (unsigned char*)memman_alloc_4k(memman, 256 * 165);/* 分配内存 */
	sheet_setbuf(sht, buf, 256, 165, -1);/* 设置图层缓冲区 */
	make_window8(buf, 256, 165, "console", 0);/* 绘制窗口 */
	make_textbox8(sht, 8, 28, 240, 128, COL8_000000);
	sht->task = open_constask(sht, memtotal);
	sht->flags |= 0x20; /*有光标*/
	return sht;
}

void close_constask(struct TASK *task){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64*1024);
	memman_free_4k(memman, (int)task->fifo.buf, 128*4);
	task->flags = 0;
	return;
}

void close_console(struct SHEET *sht){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int)sht->buf, 256*165);
	sheet_free(sht);
	close_constask(task);
	return;
}

