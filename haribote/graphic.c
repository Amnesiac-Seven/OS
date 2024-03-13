/*描画等功能*/

#include"bootpack.h"

/* 初始化调色板 */
void init_palette(void){/*RGB（红绿蓝）方式，用6位十六进制数，也就是24位指定颜色*/
	static unsigned char table_rgb[16*3]={
		0x00, 0x00, 0x00, /* 0:黑 */
		0xff, 0x00, 0x00, /* 1:亮红 */
		0x00, 0xff, 0x00, /* 2:亮绿 */
		0xff, 0xff, 0x00, /* 3:亮黄 */
		0x00, 0x00, 0xff, /* 4:亮蓝 */
		0xff, 0x00, 0xff, /* 5:亮紫 */
		0x00, 0xff, 0xff, /* 6:浅亮蓝 */
		0xff, 0xff, 0xff, /* 7:白 */
		0xc6, 0xc6, 0xc6, /* 8:亮灰 */
		0x84, 0x00, 0x00, /* 9:暗红 */
		0x00, 0x84, 0x00, /* 10:暗绿 */
		0x84, 0x84, 0x00, /* 11:暗黄 */
		0x00, 0x00, 0x84, /* 12:暗青 */
		0x84, 0x00, 0x84, /* 13:暗紫 */
		0x00, 0x84, 0x84, /* 14:浅暗蓝 */
		0x84, 0x84, 0x84  /* 15:暗灰 */
	};
	unsigned char table2[216*3];
	int r, g, b;
	set_palette(0,15,table_rgb);
	for(b = 0; b < 6; b++){
		for(g = 0; g < 6; g++){
			for(r = 0; r < 6; r++){
				table2[(b*36+g*6+r)*3+0] = r*51;
				table2[(b*36+g*6+r)*3+1] = g*51;
				table2[(b*36+g*6+r)*3+2] = b*51;
			}
		}
	}
	set_palette(16,231,table2);
	return;
}
/* 设置调色板：将rgb数组中的数据写入到start-end调色板中 */
void set_palette(int start,int end,unsigned char* rgb){
	int i,eflags;
	eflags=io_load_eflags();		/* 记录中断许可标志的值 */
	io_cli();						/* 将许可标志置为0，禁止中断 */
	io_out8(0x03c8,start);
	for(i=start; i<=end; i++){
		io_out8(0x03c9,rgb[0]/4);
		io_out8(0x03c9,rgb[1]/4);
		io_out8(0x03c9,rgb[2]/4);
		rgb+=3;
	}
	io_store_eflags(eflags);		/* 恢复许可标志的值 */
	return;
}
/* 将坐标(x0,y0)和(x1,y1)围成的长方形内的像素颜色变成c */
/* vram为VRAM位置，xsize为屏幕长度像素（在3.8节设置过，为320），c为颜色，坐标(x0,y0)和(x1,y1) */
void boxfill8(unsigned char*vram,int xsize,unsigned char c,int x0,int y0,int x1,int y1){
	int x,y;
	for(y=y0;y<=y1;y++){
		for(x=x0;x<=x1;x++){
			vram[y*xsize+x]=c;
		}
	}
	return;
}
/* 绘制320*200内的像素屏幕框架 */
void init_screen(char *vram, int xsize, int ysize){
	boxfill8(vram, xsize, COL8_008484,  0, 			0, xsize - 1, ysize - 29);// 矩形
	boxfill8(vram, xsize, COL8_C6C6C6,  0, ysize - 28, xsize - 1, ysize - 28);// 横长条
	boxfill8(vram, xsize, COL8_FFFFFF,  0, ysize - 27, xsize - 1, ysize - 27);// 横长条
	boxfill8(vram, xsize, COL8_C6C6C6,  0, ysize - 26, xsize - 1, ysize -  1);// 矩形
	
	boxfill8(vram, xsize, COL8_FFFFFF,  3, ysize - 24, 		  59, ysize - 24);// 横长条
	boxfill8(vram, xsize, COL8_FFFFFF,  2, ysize - 24, 		   2, ysize -  4);// 竖长条
	boxfill8(vram, xsize, COL8_848484,  3, ysize -  4, 		  59, ysize -  4);// 横长条
	boxfill8(vram, xsize, COL8_848484, 59, ysize - 23, 		  59, ysize -  5);// 竖长条
	boxfill8(vram, xsize, COL8_000000,  2, ysize -  3, 		  59, ysize -  3);// 横长条
	boxfill8(vram, xsize, COL8_000000, 60, ysize - 24, 		  60, ysize -  3);// 竖长条
	
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 24, xsize -  4, ysize - 24);// 长横条
	boxfill8(vram, xsize, COL8_848484, xsize - 47, ysize - 23, xsize - 47, ysize -  4);// 竖长条
	boxfill8(vram, xsize, COL8_FFFFFF, xsize - 47, ysize -  3, xsize -  4, ysize -  3);// 长横条
	boxfill8(vram, xsize, COL8_FFFFFF, xsize -  3, ysize - 24, xsize -  3, ysize -  3);// 竖长条
	return;
}

// vram为指针   xsize分辨率长度   (x,y)为字符开始显示的位置    c为颜色    font为8*16字符数组
// 将font数组绘制到VRAM中以坐标(x, y)为起点处，字符颜色为c
void putfont8(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char d,*p;
	for(i=0;i<16;i++){
		p=vram+(y+i)*xsize+x;
		d=font[i];
		if(d&0x80)p[0]=c;
		if(d&0x40)p[1]=c;
		if(d&0x20)p[2]=c;
		if(d&0x10)p[3]=c;
		if(d&0x08)p[4]=c;
		if(d&0x04)p[5]=c;
		if(d&0x02)p[6]=c;
		if(d&0x01)p[7]=c;
	}
	return;
}
// 将字符串s绘制到VRAM中以坐标(x, y)为起点处，字符颜色为c
// 逐个读入字符串中的字符，调用putfont8函数进行打印，打印完字符后需要偏移x
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s){
	extern char hankaku[4096];
	struct TASK *task = task_now();
	char *nihongo = (char *) *((int *) 0x0fe8), *font;
	int k, t;//k为区数 t为点数  每个点32字节
	if(task->langmode == 0){
		for(; *s != 0x00; s++){/*C语言中，字符串都是以0x00结尾的*/
			putfont8(vram, xsize, x, y, c, hankaku+*s*16);
			x += 8;
		}
	}
	if(task->langmode == 1){
		for(; *s != 0x00; s++){
			if(task->langbyte1 == 0){
				if((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)){//全角字符
					task->langbyte1 = *s;
				}else{
					putfont8(vram, xsize, x, y, c, nihongo+*s*16);
				}
			}else{
				if(0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f){
					k = (task->langbyte1-0x81)*2;
				}else{
					k = (task->langbyte1-0xe0)*2+62;
				}
				if(0x40 <= *s && *s <= 0x7e){//全角较小区
					t = *s - 0x40;
				}else if (0x80 <= *s && *s <= 0x9e){//全角较小区
					t = *s - 0x80 + 63;
				}else{//全角较大区
					t = *s - 0x9f;
					k++;
				}
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;//1个区中包含94个点
				putfont8(vram, xsize, x - 8, y, c, font     ); /*左半部分*/
				putfont8(vram, xsize, x    , y, c, font + 16); /*右半部分*/
			}
			x += 8;
		}
	}
	if(task->langmode == 2){
		for(; *s != 0x00; s++){
			if(task->langbyte1 == 0){
				if((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)){//全角字符
					task->langbyte1 = *s;
				}else{
					putfont8(vram, xsize, x, y, c, nihongo+*s*16);
				}
			}else{
				k = task->langbyte1 - 0xa1;
				t = *s - 0xa1;
				task->langbyte1 = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;//1个区中包含94个点
				putfont8(vram, xsize, x - 8, y, c, font     ); /*左半部分*/
				putfont8(vram, xsize, x    , y, c, font + 16); /*右半部分*/
			}
			x += 8;
		}
	}
	return;
}

// 按照鼠标形状进行显示，其中mouse为表示鼠标形状的字符串，bc为某一种背景色
void init_mouse_cursor8(char *mouse, char bc){
	static char cursor[16][16]={
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	int x, y;
	for(y=0;y<16;y++){
		for(x=0;x<16;x++){
			if(cursor[y][x]=='*')mouse[y*16+x]=COL8_000000;
			else if(cursor[y][x]=='O')mouse[y*16+x]=COL8_FFFFFF;
			else if(cursor[y][x]=='.')mouse[y*16+x]=bc;
		}
	}
	return;
}
// 从buf读出pxsize*pysize个图形数据(bxsize指定读出哪些连续数据的)，并将该图形写到vram的(px0,py0)坐标处
// vram=0xa0000  vxsize=320  pxsize和pysize是想要显示的图形（picture）的大小
// px0和py0指定图形在画面上的显示位置  buf和bxsize分别指定图形的存放地址和每一行含有的像素数
void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, 
		int px0, int py0, char *buff, int bxsize){
	int x, y;
	for(y=0;y<pysize;y++){
		for(x=0;x<pxsize;x++)
			vram[(py0+y)*vxsize+px0+x] = buff[y*bxsize+x];
	}
	return;
}

