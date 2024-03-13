#include "apilib.h"

struct DLL_STRPICENV {	/* 64KB */
	int work[64 * 1024 / 4];
};

struct RGB {
	unsigned char b, g, r, t;
};

/* bmp.nasm */
int info_BMP(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_BMP(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

/* jpeg.c */
int info_JPEG(struct DLL_STRPICENV *env, int *info, int size, char *fp);
int decode0_JPEG(struct DLL_STRPICENV *env, int size, char *fp, int b_type, char *buf, int skip);

unsigned char rgb2pal(int r, int g, int b, int x, int y);
void error(char *s);

void HariMain(void)
{
	struct DLL_STRPICENV env;
	char filebuf[512 * 1024], winbuf[1040 * 805];
	char s[32], *p;
	int win, i, j, fsize, xsize, info[8];
	struct RGB picbuf[1024 * 768], *q;

	/* 命令行解析 */
	api_cmdline(s, 30);
	for (p = s; *p > ' '; p++);	/* 找到空格 */
	for (; *p == ' '; p++);	/* 剔除空格 */

	/* 文件内容?入 */
	i = api_fopen(p); 
	if (i == 0)
		error("file not found.\n"); 
	fsize = api_fsize(i, 0);
	if (fsize > 512 * 1024)
		error("file too large.\n");
	api_fread(filebuf, fsize, i);
	api_fclose(i);

	/* ?像文件?型 */
	if(info_BMP(&env, info, fsize, filebuf) == 0){
		/* 不是BMP */
		if(info_JPEG(&env, info, fsize, filebuf) == 0){
			/* 也不是JPEG */
			api_putstr0("file type unknown.\n");
			api_end();
		}
	}
	/* 上面其中一个info函数?用成功的?，info中包含以下信息 */
	/*	info[0] : 文件?型(1:BMP, 2:JPEG) */
	/*	info[1] : ?色数信息*/
	/*	info[2] : xsize */
	/*	info[3] : ysize */

	if(info[2] > 1024 || info[3] > 768){
		error("picture too large.\n");
	}

	/* 窗口准? */
	xsize = info[2] + 16;
	if(xsize < 136){
		xsize = 136;
	}
	win = api_openwin(winbuf, xsize, info[3] + 37, -1, "gview");

	/* 根据文件内容filebuf??成?像数据picbuf */
	if(info[0] == 1){
		i = decode0_BMP (&env, fsize, filebuf, 4, (char *) picbuf, 0);
	}else{
		i = decode0_JPEG(&env, fsize, filebuf, 4, (char *) picbuf, 0);
	}
	/* b_type = 4 表示struct RGB格式 */
	/* skip??0即可 */
	if(i != 0){
		error("decode error.\n");
	}

	/* ?示 */
	for(i = 0; i < info[3]; i++){
		p = winbuf + (i + 29) * xsize + (xsize - info[2]) / 2;//?算?示的x位置(居中?示)
		q = picbuf + i * info[2];//?片中某一行中的第一个像素点
		for(j = 0; j < info[2]; j++){
			p[j] = rgb2pal(q[j].r, q[j].g, q[j].b, j, i);
		}
	}
	api_refreshwin(win, (xsize - info[2]) / 2, 29, (xsize - info[2]) / 2 + info[2], 29 + info[3]);

	/* 等待?束 */
	for (;;) {
		i = api_getkey(1);
		if (i == 'Q' || i == 'q') {
			api_end();
		}
	}
}
// 将RGB三色各分成6个色度，也就是6 * 6 * 6 = 216?
// 而前16??色已被?定，所以?216??色的色号从下?16?始
unsigned char rgb2pal(int r, int g, int b, int x, int y)
{
	static int table[4] = { 3, 1, 0, 2 };
	int i;
	x &= 1; /* 0或1 */
	y &= 1;
	i = table[x + y * 2];	/* 0~3 */
	r = (r * 21) / 256;	/* 随机数0~20 */
	g = (g * 21) / 256;
	b = (b * 21) / 256;
	r = (r + i) / 4;	/* 随机数0~5 */
	g = (g + i) / 4;
	b = (b + i) / 4;
	return 16 + r + g * 6 + b * 36;
}

void error(char *s)
{
	api_putstr0(s);
	api_end();
}
