#include "bootpack.h"
#include <stdio.h>
#include <string.h>

void console_task(struct SHEET *sheet, unsigned int memtotal){
	struct TASK *task = task_now();
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEHANDLE fhandle[8];
	int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	unsigned char *nihongo = (char *) *((int *) 0x0fe8);
	struct CONSOLE cons;
	char cmdline[30], s;
	cons.sht = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_c = -1;
	task->cons = &cons;
	task->cmdline = cmdline;
	
	if(cons.sht != 0){
		cons.timer = timer_alloc();// 分配一个计时器
		timer_init(cons.timer, &task->fifo, 1);// 计时器初始化
		timer_settime(cons.timer, 50);// 设置计时器中断时长
	}
	//从0柱面、0磁头、2扇区在磁盘映像中相当于0x000200
	file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
	
	for(i = 0; i < 8; i++){
		fhandle[i].buf = 0;
	}
	task->fhandle = fhandle;
	task->fat = fat;
	
	if(nihongo[4096] != 0xff){/*是否载入了日文字库？*/
		task->langmode = 1;
	}else{
		task->langmode = 0;
	}
	task->langbyte1 = 0;
	
	/*显示提示符*/
	cons_putchar(&cons, '>', 1);
	for(;;){
		io_cli();
		if(fifo32_status(&task->fifo) == 0){
			task_sleep(task);
			io_sti();
		}
		else{
			i = fifo32_get(&task->fifo);
			io_sti();
			if(i <= 1 && cons.sht != 0){
				if(i != 0){
					timer_init(cons.timer, &task->fifo, 0);
					if(cons.cur_c >= 0)
						cons.cur_c = COL8_FFFFFF;
				}else{
					timer_init(cons.timer, &task->fifo, 1);
					if(cons.cur_c >= 0)
						cons.cur_c = COL8_000000;
				}
				timer_settime(cons.timer, 50);/* 重新设定计时器 */
			}
			if(i == 2){/*命令行光标ON*/
				cons.cur_c = COL8_FFFFFF;
			}
			if(i == 3){/*命令行光标OFF*/
				if(cons.sht != 0)
					boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, 
						cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);/*用黑色直接覆盖光标位置*/
				cons.cur_c = -1;
			}
			if(i == 4){
				cmd_exit(&cons, fat);
			}
			if(256 <= i && i <= 511){ /*键盘数据*/
				if(i == 8+256){/*退格键*/
					if(cons.cur_x > 16){/*用空白擦除光标后将光标前移一位*/
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
					}
				}else if(i == 10+256){/*回车键*/
					cons_putchar(&cons, ' ', 0);/*空白擦除光标*/
					cmdline[cons.cur_x / 8 - 2] = 0;//-2的原因是cons.cur_x初始值为16
					cons_newline(&cons);
					cons_runcmd(cmdline, &cons, fat, memtotal); /*运行命令*/
					if(cons.sht == 0)
						cmd_exit(&cons, fat);
					cons_putchar(&cons, '>', 1);
				}else{/*一般字符*/
					if(cons.cur_x < 240){/*显示一个字符之后将光标后移一位 */
						cmdline[cons.cur_x / 8 - 2] = i - 256;
						cons_putchar(&cons, i - 256, 1);
					}
				}
			}
			if(cons.sht != 0){
				if(cons.cur_c >= 0){
					boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, 
						cons.cur_x, cons.cur_y,  cons.cur_x+7, cons.cur_y+15);
				}
				sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x+8, cons.cur_y+16);					
			}
		}
	}
}
//实现换行功能
void cons_newline(struct CONSOLE *cons){
	int x, y;
	struct SHEET *sheet = cons->sht;
	struct TASK *task = task_now();
	if(cons->cur_y < 28+112){
		cons->cur_y += 16;/*换行*/
	}else{/*滚动*/
		if(sheet != 0){
			for(y = 28; y < 28+112; y++){//字符覆盖
				for(x = 8;x < 8+240; x++){
					sheet->buf[x+y*sheet->bxsize] = sheet->buf[x+(y+16)*sheet->bxsize];
				}
			}
			for(y = 28+112; y < 28+128; y++){//新增的一行涂黑
				for(x = 8;x < 8+240; x++){
					sheet->buf[x+y*sheet->bxsize] = COL8_000000;
				}
			}
			sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);	
		}
	}
	cons->cur_x = 8;
	if(task->langmode == 1 && task->langbyte1 != 0){
		cons->cur_x += 8;
	}
	return;
}
//识别命令，执行相应操作
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal){
	if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
		cmd_mem(cons, memtotal);
	}else if (strcmp(cmdline, "cls") == 0 && cons->sht != 0) {
		cmd_cls(cons);
	}else if (strcmp(cmdline, "dir") == 0 && cons->sht != 0) {
		cmd_dir(cons);
	}else if (strcmp(cmdline, "exit") == 0) {
		cmd_exit(cons, fat);
	}else if (strncmp(cmdline, "start ", 6) == 0) {
		cmd_start(cons, cmdline, memtotal);
	}else if (strncmp(cmdline, "ncst ", 5) == 0) {
		cmd_ncst(cons, cmdline, memtotal);
	}else if (strncmp(cmdline, "langmode ", 9) == 0) {
		cmd_langmode(cons, cmdline);
	}else if (cmdline[0] != 0) {
		/*不是命令，也不是空行 */
		if(cmd_app(cons, fat, cmdline) == 0){
			cons_putstr0(cons, "Bad command.\n\n");
		}
	}
	return;
}
//显示内存总量和剩余内存
void cmd_mem(struct CONSOLE *cons, unsigned int memtotal){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char s[30];
	sprintf(s, "total %dMB\nfree %dKB\n\n", memtotal/(1024*1024),memman_total(memman)/1024);
	cons_putstr0(cons, s);
	return;
}
//清屏操作
void cmd_cls(struct CONSOLE *cons){//将文本内所有像素点全部置为黑色，
	int x, y;
	struct SHEET *sheet = cons->sht;
	for (y = 28; y < 28 + 128; y++) {
		for (x = 8; x < 8 + 240; x++) {
			sheet->buf[x + y * sheet->bxsize] = COL8_000000;
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);//刷新命令窗口
	cons->cur_y = 28;
	return;
}
//打印文件信息
void cmd_dir(struct CONSOLE *cons){
	struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
	int i, j;
	char s[30];
	for (i = 0; i < 224; i++) {//文件信息最多可以存放224个
		/*如果文件名的第一个字节为0xe5，代表这个文件已经被删除了
		  文件名第一个字节为0x00，代表这一段不包含任何文件名信息*/
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {//排除非文件信息和目录
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j = 0; j < 8; j++) {
					s[j] = finfo[i].name[j];
				}
				s[ 9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
	return;
}

void cmd_exit(struct CONSOLE *cons, int *fat){
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_now();
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
	if(cons->sht != 0)
		timer_cancel(cons->timer);//取消控制光标闪烁的定时器
	memman_free_4k(memman, (int) fat, 4 * 2880);//FAT用的内存空间释放
	io_cli();
	if(cons->sht != 0)//有命令行窗口时，通过图层的地址告诉task_a需要结束哪个任务
		fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768); /* 768~1023 */
	else//将TASK结构的地址告诉task_a
		fifo32_put(fifo, task - taskctl->tasks0 + 1024);
	io_sti();
	for (;;) {
		task_sleep(task);
	}
}
// 创建新的命令窗口，并显示
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal){
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	for(i = 6; cmdline[i] != 0; i++){
		/*将命令行输入的字符串逐字复制到新的命令行窗口中*/
		fifo32_put(fifo, cmdline[i]+256);
	}
	fifo32_put(fifo, 10+256);/*回车键*/
	cons_newline(cons);
	return;
}
// 仅创建新的命令窗口，不显示
// 没有窗口的命令行任务的cons—>sht规定为0
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal){
	struct SHEET *sht = open_console(0, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	for(i = 5; cmdline[i] != 0; i++){
		/*将命令行输入的字符串逐字复制到新的命令行窗口中*/
		fifo32_put(fifo, cmdline[i]+256);
	}
	fifo32_put(fifo, 10+256);/*回车键*/
	cons_newline(cons);
	return;
}
//修改语言模式
void cmd_langmode(struct CONSOLE *cons, char *cmdline){
	struct TASK *task = task_now();
	unsigned char mode = cmdline[9]-'0';
	if(mode <= 2){
		task->langmode = mode;
	}else{
		cons_putstr0(cons, "mode number error.\n");
	}
	cons_newline(cons);
	return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline){
	int i, segsiz, datsiz, esp, dathrb, appsiz;
	struct SHEET *sht;
	struct SHTCTL *shtctl;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct FILEINFO *finfo;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	char name[18], *p, *q;
	struct TASK *task = task_now();
	for(i = 0; i < 13; i++){//将cmdline转换成name
		if(cmdline[i] <= ' ')break;//空格32 换行回车等操作的ASCII码均小于32
		name[i] = cmdline[i];
	}
	name[i] = 0;
	finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);//寻找与name相同的文件
	if(finfo == 0 && name[i-1] != '.'){//没找到，则增加后缀名再次查找
		name[i  ] = '.';
		name[i+1] = 'H';
		name[i+2] = 'R';
		name[i+3] = 'B';
		name[i+4] = 0;
		finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
	}
	if (finfo != 0) {
		/* 找到了与字符串相同的文件 */
		appsiz = finfo->size;
		p = file_loadfile2(finfo->clustno, &appsiz, fat);
		if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00){
			segsiz = *((int*)(p+0x0000));
			esp    = *((int*)(p+0x000c));//数据传送目的地址
			datsiz = *((int*)(p+0x0010));//数据段部分大小
			dathrb = *((int*)(p+0x0014));//放在hrb文件中的起始位置，复制从该地址起的datsiz个数据
			q = (char *) memman_alloc_4k(memman, segsiz);
			task->ds_base = (int)q;
			set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER+0x60);
			set_segmdesc(task->ldt + 1, segsiz - 1, (int) q, AR_DATA32_RW+0x60);
			for(i = 0; i < datsiz; i++)/*将.hrb文件中的数据部分先复制到数据段*/
				q[esp+i] = p[dathrb+i];
			start_app(0x1b, 0*8+4, esp, 1*8+4, &(task->tss.esp0));
			shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
			for(i = 0; i < MAX_SHEETS; i++){
				sht = &(shtctl->sheets0[i]);
				if((sht->flags & 0x11) == 0x11 && sht->task == task)
					sheet_free(sht);
			}
			for(i = 0; i < 8; i++){
				if(task->fhandle[i].buf != 0){
					memman_free_4k(memman, (int)task->fhandle[i].buf, task->fhandle[i].size);
					task->fhandle[i].buf = 0;
				}
			}
			timer_cancelall(&task->fifo);
			memman_free_4k(memman, (int) q, segsiz);
			task->langbyte1 = 0;
		}else
			cons_putstr0(cons, ".hrb file format error.\n");
		memman_free_4k(memman, (int) p, appsiz);
		cons_newline(cons);
		return 1;
	}
	return 0;
}
/*显示提示符*/
void cons_putchar(struct CONSOLE *cons, int chr, char move){
	char s[2];
	s[0] = chr;
	s[1] = 0;
	if(s[0] == 0x09){/*制表符*/
		for(;;){
			if(cons->sht != 0)
				putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
			cons->cur_x += 8;
			if(cons->cur_x == 8+240){//换行打印
				cons_newline(cons);
			}
			if (((cons->cur_x-8) & 0x1f) == 0)break;//被32整除则退出，打印多个空格（不一定是4个），用于表示一个制表符
		}
	}else if(s[0] == 0x0a){/*换行*/
		cons_newline(cons);
	}else if(s[0] == 0x0b){/*回车*/
		
	}else{/*一般字符*/
		if(cons->sht != 0)
			putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
		if(move != 0){
			cons->cur_x += 8;
			if(cons->cur_x == 8+240){//整行输出
				cons_newline(cons);
			}	
		}
	}
	return;
}
//打印字符串
void cons_putstr0(struct CONSOLE *cons, char *s){
	for(; *s != 0; s++)
		cons_putchar(cons, *s, 1);
	return;
}
//打印s中前l个字符
void cons_putstr1(struct CONSOLE *cons, char *s, int l){
	int i;
	for(i = 0; i < l; i++)
		cons_putchar(cons, s[i], 1);
	return;
}
//根据输入参数打印数据 eax为当前字符 ebx为字符串地址 ecx长度
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax){
	int i;
	struct TASK *task = task_now();
	int ds_base = task->ds_base;
	struct CONSOLE *cons = task->cons;
	struct FILEINFO *finfo;
	struct FILEHANDLE *fh;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
	struct SHEET *sht;
	int *reg = &eax + 1;/* eax后面的地址*/
	/* reg[0] : EDI, reg[1] : ESI, reg[2] : EBP, reg[3] : ESP */
	/* reg[4] : EBX, reg[5] : EDX, reg[6] : ECX, reg[7] : EAX */
	if(edx == 1)//打印字符
		cons_putchar(cons, eax&0xff, 1);
	else if(edx == 2)//打印字符串
		cons_putstr0(cons, (char*)ebx+ds_base);
	else if(edx == 3)//打印字符串中的前ecx个字符
		cons_putstr1(cons, (char*)ebx+ds_base, ecx);
	else if(edx == 4)//返回
		return &(task->tss.esp0); 
	else if(edx == 5){//创建窗口
		sht = sheet_alloc(shtctl);
		sht->task = task;
		sht->flags |= 0x10;
		sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
		make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
		sheet_slide(sht, ((shtctl->xsize-esi)/2)&~3, (shtctl->ysize-edi)/2);
		sheet_updown(sht, shtctl->top); /*将窗口图层高度指定为当前鼠标所在图层的高度，鼠标移到上层*/
		reg[7] = (int) sht; 
	}else if(edx == 6){
		sht = (struct SHEET *) (ebx&0xfffffffe);
		putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
		if((ebx&1) == 0)
			sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
	}else if(edx == 7){
		sht = (struct SHEET *) (ebx&0xfffffffe);
		boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
		if((ebx&1) == 0)
			sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
	}else if(edx == 8){//memman初始化
		memman_init((struct MEMMAN*)(ebx+ds_base));
		ecx&=0xfffffff0;//所管理的内存空间的字节数
		memman_free((struct MEMMAN*)(ebx+ds_base), eax, ecx);//eax内存空间的起始地址
	}else if(edx == 9){//malloc
		ecx = (ecx+0x0f)&0xfffffff0;//所管理的内存空间的字节数
		reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);//eg[7] : EAX
	}else if(edx == 10){//free
		ecx = (ecx+0x0f)&0xfffffff0;//所释放的内存空间的字节数
		memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
	}else if(edx == 11){//画点
		sht = (struct SHEET *) (ebx&0xfffffffe);
		sht->buf[sht->bxsize * edi + esi] = eax;
		if((ebx&1) == 0)//当ebx的最低位为0，也就是没有字符串需要打印时，刷新图层
			sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
	}else if(edx == 12){//更新图层
		sht = (struct SHEET *) ebx;
		sheet_refresh(sht, eax, ecx, esi, edi);
	}else if(edx == 13){//画直线
		sht = (struct SHEET *) (ebx&0xfffffffe);
		hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
		if((ebx&1) == 0){//当ebx的最低位为0，也就是没有字符串需要打印时，刷新图层
			if(eax > esi){
				i = eax;
				eax = esi;
				esi = i;
			}
			if(ecx > edi){
				i = ecx;
				ecx = edi;
				edi = i;
			}
			sheet_refresh(sht, eax, ecx, esi, edi);
		}
	}else if(edx == 14){//关闭窗口
		sheet_free((struct SHEET *) ebx);
	}else if(edx == 15){
		for(;;){
			io_cli();
			if(fifo32_status(&task->fifo) == 0){
				if(eax != 0)task_sleep(task);//休眠
				else{//没有键盘输入时返回-1，不休眠
					io_sti();
					reg[7] = -1;
					return 0;
				}
			}
			i = fifo32_get(&task->fifo);
			if(i <= 1){//光标
				/*应用程序运行时不需要显示光标，因此总是将下次显示用的值置为1*/
				timer_init(cons->timer, &task->fifo, 1);
				timer_settime(cons->timer, 50);
			}
			if(i == 2)cons->cur_c = COL8_FFFFFF;/*光标ON */
			if(i == 3)cons->cur_c = -1;/*光标OFF */
			if(i == 4){/*只关闭命令行窗口*/
				timer_cancel(cons->timer);
				io_cli();
				fifo32_put(sys_fifo, cons->sht-shtctl->sheets0+2024);
				cons->sht = 0;
				io_sti();
			}
			if(256 <= i){//键盘数据
				reg[7] = i-256;
				return 0;
			}
		}
	}else if(edx == 16){//分配计时器
		reg[7] = (int)timer_alloc();
		((struct TIMER *) reg[7])->flags2 = 1;
	}else if(edx == 17){//初始化计时器
		//是因为在向应用程序传递FIFO数据时，需要先减去256
		timer_init((struct TIMER *)ebx, &task->fifo, eax+256);
	}else if(edx == 18){//设置计时器
		timer_settime((struct TIMER *)ebx, eax);
	}else if(edx == 19){//释放计时器
		timer_free((struct TIMER *)ebx);
	}else if(edx == 20){//蜂鸣器
		if(eax == 0){//停止发声 蜂鸣器OFF
			i = io_in8(0x61);
			io_out8(0x61, i&0x0d);
		}else{
			//设置声音频率
			i = 1193180000/eax;
			io_out8(0x43, 0xb6);
			io_out8(0x42, i&0xff);
			io_out8(0x42, i >> 8);
			//蜂鸣器ON
			i = io_in8(0x61);
			io_out8(0x61, (i|0x03)&0x0f);
		}
	}else if(edx == 21){//打开文件
		for(i = 0; i < 8; i++){
			if(task->fhandle[i].buf == 0)
				break;
		}
		fh = &task->fhandle[i];
		reg[7] = 0;
		if(i < 8){
			finfo = file_search((char*)ebx+ds_base, (struct FILEINFO*)(ADR_DISKIMG+0x002600), 224);
			if(finfo != 0){
				reg[7] = (int)fh;
				fh->size = finfo->size;
				fh->pos = 0;
				fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
			}
		}
	}else if(edx == 22){//关闭文件
		fh = (struct FILEHANDLE*)eax;
		memman_free_4k(memman, (int)fh->buf, fh->size);//与打开文件操作中的memman_alloc_4k对应
		fh->buf = 0;
	}else if(edx == 23){//文件定位
		fh = (struct FILEHANDLE*)eax;
		if(ecx == 0){//定位的起点为文件开头
			fh->pos = ebx;
		}else if(ecx == 1){//定位的起点为当前的访问位置
			fh->pos += ebx;
		}else if(ecx == 2){//定位的起点为文件末尾
			fh->pos = fh->size + ebx;
		}
		if(fh->pos < 0)fh->pos = 0;
		if(fh->pos > fh->size)fh->pos = fh->size;
	}else if(edx == 24){//获取文件大小
		fh = (struct FILEHANDLE*)eax;
		if(ecx == 0){//普通文件大小
			reg[7] = fh->size;
		}else if(ecx == 1){//当前读取位置从文件开头起算的偏移量
			reg[7] = fh->pos;
		}else if(ecx == 2){//当前读取位置从文件末尾起算的偏移量
			reg[7] = fh->pos - fh->size;
		}
	}else if(edx == 25){//文件读取,写入到ebx缓存区中
		fh = (struct FILEHANDLE*)eax;
		for(i = 0; i < ecx; i++){
			if(fh->pos == fh->size)break;
			*((char *) ebx + ds_base + i) = fh->buf[fh->pos];
			fh->pos++;
		}
		reg[7] = i;
	}else if(edx == 26){//获取命令行,将命令行
		i = 0;
		for(;;){
			*((char *) ebx + ds_base + i) = task->cmdline[i];
			if(task->cmdline[i] == 0)break;
			if(i >= ecx)break;
			i++;
		}
		reg[7] = i;
	}else if(edx == 27){
		reg[7] = task->langmode;
	}
	return 0;
}
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1,int col){
	int i, x, y, len, dx, dy;
	dx = x1-x0;
	dy = y1-y0;
	x = x0<<10;
	y = y0<<10;
	if(dx < 0)dx = -dx;
	if(dy < 0)dy = -dy;
	if(dx >= dy){
		len = dx+1;
		if(x0 > x1)dx = -1024;
		else dx = 1024;
		if(y0 <= y1)dy = ((y1-y0+1)<<10)/len;
		else dy = ((y1-y0-1)<<10)/len;//-(y0-y1+1) = y1-y0-1
	}else{
		len = dy+1;
		if(y0 > y1)dy = -1024;
		else dy = 1024;
		if(x0 <= x1)dx = ((x1-x0+1)<<10)/len;
		else dx = ((x1-x0-1)<<10)/len;
	}
	for(i = 0; i < len; i++){
		sht->buf[(y>>10)*sht->bxsize+(x>>10)] = col;
		x += dx;
		y += dy;
	}
	return;
}
//栈异常
int *inthandler0c(int *esp){
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n Stack Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0); /*强制结束程序*/
}
// 一般性保护异常
int *inthandler0d(int *esp){
	struct TASK *task = task_now();
	struct CONSOLE *cons = task->cons;
	char s[30];
	cons_putstr0(cons, "\nINT 0D :\n General Protected Exception.\n");
	sprintf(s, "EIP = %08X\n", esp[11]);
	cons_putstr0(cons, s);
	return &(task->tss.esp0); /*强制结束程序*/
}
