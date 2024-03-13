#include "apilib.h"
#include <stdio.h>
#include <string.h>

int strtol(char *s, char **endp, int base); /*标准函数(stdlib.h) */
char *skipspace(char *p);
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang);
char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang);
int puttab(int x, int w, int xskip, char *s, int tab);

void HariMain(void)
{
	char winbuf[1024 * 757], txtbuf[240 * 1024];
	int w = 30, h = 10, t = 4, spd_x = 1, spd_y = 1;
	int win, i, j, lang = api_getlang(), xskip = 0;
	char s[30], *p, *q = 0, *r = 0;
	
	api_cmdline(s, 30);
	for(p = s; *p > ' '; p++);//去掉s字符串前面的空格
	for(; *p != 0;){//先剔除掉前面的设置部分
		p = skipspace(p);
		if(*p == '-'){
			if(p[1] == 'w'){
				w = strtol(p + 2, &p, 0);
				if(w < 20)w = 20;
				if(w > 126) w = 126;
			}else if(p[1] == 'h'){
				h = strtol(p + 2, &p, 0);
				if(h < 1)h = 1;
				if(h > 45) h = 45;
			}else if(p[1] == 't'){
				t = strtol(p + 2, &p, 0);
				if(t < 1)t = 4;
			}else{
	err:
				api_putstr0(" >tview file [-w30 -h10 -t4]");
				api_end();
			}
		}else{/*找到以q为首字符串的文件名*/
			if(q != 0)goto err;
			q = p;
			for(; *p > ' '; p++);
			r = p;
		}
		if(q == 0)goto err;
		
		/*准备窗口*/
		win = api_openwin(winbuf, w * 8 + 16, h * 16 + 37, -1, "tview");
		api_boxfilwin(win, 6, 27, w * 8 + 9, h * 16 + 30, 7);
		
		/*载入文件*/
		*r = 0;
		i = api_fopen(q);
		if(i == 0){
			api_putstr0("file open error!");
			api_end();
		}
		j = api_fsize(i, 0);
		if(j >= 1024 * 240 - 1)j = 1024 * 240 - 2;//规定展示的文件大小
		txtbuf[0] = 0x0a;
		api_fread(txtbuf + 1, j, i);//将文件读到txtbuf中
		api_fclose(i);
		txtbuf[j + 1] = 0;
		q = txtbuf + 1;
		for(p = txtbuf + 1; *p != 0; p++){/*为了让处理变得简单，删掉0x0d的代码*/
			if(*p != 0x0d){
				*q = *p;
				q++;
			}
		}
		*q = 0;
		
		/*主体*/
		p = txtbuf + 1;
		for(;;){
			textview(win, w, h, xskip, p, t, lang);
			i = api_getkey(1);
			if(i == 'Q' || i == 'q')api_end();
			if('A' <= i && i <= 'F'){
				spd_x = 1 << (i - 'A');/* 1, 2, 4, 8, 16, 32 */
			}
			if('a' <= i && i <= 'f'){
				spd_y = 1 << (i - 'a');/* 1, 2, 4, 8, 16, 32 */
			}
			if(i == '<' && t > 1){
				t /= 2;
			}
			if (i == '>' && t < 256) {
				t *= 2;
			}
			if(i == '4'){/*按下'4'左移，如果没有按下“4”则处理结束*/
				for(;;){
					xskip -= spd_x;
					if(xskip < 0)xskip = 0;
					if(api_getkey(0) != '4')break;
				}
			}
			if(i == '6'){
				for(;;){
					xskip += spd_x;
					if(api_getkey(0) != '6')break;
				}
			}
			if(i == '8'){
				for(;;){
					for(j = 0; j < spd_y; j++){
						if(p == txtbuf + 1)break;
						for(p--; p[-1] != 0x0a; p--);/*回溯到上一个字符为0x0a为止*/
					}
					if(api_getkey(0) != '8')break;		
				}
			}
			if(i == '2'){
				for(;;){
					for(j = 0; j < spd_y; j++){/*检查显示的文本是否是最后一行*/
						for(q = p; *q != 0 && *q != 0x0a; q++);
						if(*q == 0)break;
						p = q + 1;
					}
					if(api_getkey(0) != '2')break;		
				}
			}
		}
	}
}

char *skipspace(char *p){
	for(; *p == ' '; p++);
	return p;
}

void textview(int win, int w, int h, int xskip, char *p, int tab, int lang){
	int i;
	api_boxfilwin(win, 8, 29, w * 8 + 7, h * 16 + 28, 7);//用色号7覆盖区域
	for(i = 0; i < h; i++)
		p = lineview(win, w, i * 16 + 29, xskip, p, tab, lang);
	api_refreshwin(win, 8, 29, w * 8 + 8, h * 16 + 29);
	return;
}
//根据lang语言格式，从p中查看从xskip下标开始的w个字符，并将其显示
char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang){
	int x = -xskip;
	char s[130];
	for(;;){
		if(*p == 0)break;
		if(*p == 0x0a){//换行符
			p++;
			break;
		}
		if(lang == 0){/* ASCII */
			if(*p == 0x09){/*TAB键*/
				x = puttab(x, w, xskip, s, tab);
			}else{
				if(0 <= x && x < w)
					s[x] = *p;
				x++;
			}
			p++;
		}
		if(lang == 1){
			if(*p == 0x09){/*TAB键*/
				x = puttab(x, w, xskip, s, tab);
				p++;
			}else if((0x81 <= *p && *p <= 0x9f) || (0xe0 <= *p && *p <= 0xfc)){/*全角字符*/
				if(x == -1)//避免全角字符(由两个半角字符组成)只写入前半个字符
					s[0] = ' ';
				if(0 <= x && x < w - 1){
					s[x] = *p;
					s[x + 1] = p[1];
				}
				if(x == w - 1){
					s[x] = ' ';
				}
				x += 2;
				p += 2;
			}else{
				if(0 <= x && x < w)
					s[x] = *p;
				x++;
				p++;
			}
		}
		if(lang == 2){
			if(*p == 0x09){
				x = puttab(x, w, xskip, s, tab);
				p++;
			}else if(0xa1 <= *p && *p <= 0xfe){
				if(x == -1)
					s[0] = ' ';
				if(0 <= x && x < w - 1){
					s[x] = *p;
					s[x + 1] = p[1];
				}
				if(x == w - 1){
					s[x] = ' ';
				}
				x += 2;
				p += 2;
			}else{
				if(0 <= x && x < w)
					s[x] = *p;
				x++;
				p++;
			}
		}
	}
	if(x > w)x = w;
	if(x > 0){
		s[x] = 0;
		api_putstrwin(win, 8, y, 0, x, s);
	}
	return p;
}

int puttab(int x, int w, int xskip, char *s, int tab){
	for(;;){
		if(0 <= x && x < w){
			s[x] = ' ';
		}
		x++;
		if((x + xskip) % tab == 0)
			break;
	}
	return x;
}
