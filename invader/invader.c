#include "apilib.h"
#include <stdio.h>
#include <string.h>

void putstr(int win, char *winbuf, int x, int y, int col, unsigned char *s);
void wait(int i, int timer, char *keyflag);

/* 敌机invader:"abcd", 自机fighter:"efg", 炮弹laser:"h" */
static unsigned char charset[16 * 8] = {
	/* invader(0) */
	0x00, 0x00, 0x00, 0x43, 0x5f, 0x5f, 0x5f, 0x7f,
	0x1f, 0x1f, 0x1f, 0x1f, 0x00, 0x20, 0x3f, 0x00,
	/* invader(1) */
	0x00, 0x0f, 0x7f, 0xff, 0xcf, 0xcf, 0xcf, 0xff,
	0xff, 0xe0, 0xff, 0xff, 0xc0, 0xc0, 0xc0, 0x00,
	/* invader(2) */
	0x00, 0xf0, 0xfe, 0xff, 0xf3, 0xf3, 0xf3, 0xff,
	0xff, 0x07, 0xff, 0xff, 0x03, 0x03, 0x03, 0x00,
	/* invader(3) */
	0x00, 0x00, 0x00, 0xc2, 0xfa, 0xfa, 0xfa, 0xfe,
	0xf8, 0xf8, 0xf8, 0xf8, 0x00, 0x04, 0xfc, 0x00,
	/* fighter(0) */
	0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x43, 0x47, 0x4f, 0x5f, 0x7f, 0x7f, 0x00,
	/* fighter(1) */
	0x18, 0x7e, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0xff,
	0xff, 0xff, 0xe7, 0xe7, 0xe7, 0xe7, 0xff, 0x00,
	/* fighter(2) */
	0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0xc2, 0xe2, 0xf2, 0xfa, 0xfe, 0xfe, 0x00,
	/* laser */
	0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00
};
	
void HariMain(void)
{
	/*自机坐标位置fx，炮弹位置(lx, ly)*/
	/*敌机坐标位置(ix, iy)*/
	int win, timer, i, j, fx, laserwait, lx = 0, ly;
	int ix, iy, movewait0, movewait, idir;
	int invline, score, high, point;
	char winbuf[336 * 261], invstr[32 * 6], s[12], keyflag[4], *p;
	static char invstr0[32] = " abcd abcd abcd abcd abcd ";/*敌机*/
	
	win = api_openwin(winbuf, 336, 261, -1, "invader");//创建窗口
	api_boxfilwin(win, 6, 27, 329, 254, 0);//绘制中间显示部分
	timer = api_alloctimer();//申请定时器
	api_inittimer(timer, 128);//初始化定时器
	
	high = 0;
	putstr(win, winbuf, 22, 0, 7, "HIGH:00000000");
	
restart:
	score = 0;
	point = 1;
	putstr(win, winbuf, 4, 0, 7, "SCORE:00000000");
	movewait0 = 20;
	fx = 18;
	putstr(win, winbuf, fx, 13, 6, "efg");/*绘制自机*/
	wait(100, timer, keyflag);//1s间隙
	
next_group:
	wait(100, timer, keyflag);
	ix = 7;
	iy = 1;
	invline = 6;
	for(i = 0; i < 6; i++){//绘制6排敌机
		for(j = 0; j < 27; j++){
			invstr[i * 32 + j] = invstr0[j];
		}
		putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);
	}
	keyflag[0] = 0;
	keyflag[1] = 0;
	keyflag[2] = 0;
	
	ly = 0;
	laserwait = 0;//当这个变量变为0时外星人群前进一步
	movewait = movewait0;//movewait的初始值
	idir = 1;//左右移动方向
	wait(100, timer, keyflag);
	
	for(;;){
		if(laserwait != 0){
			laserwait--;
			keyflag[2] = 0;
		}
		wait(4, timer, keyflag);
		
		/*自机的处理*/
		if(keyflag[0] != 0 && fx > 0){//自机左移
			fx--;
			putstr(win, winbuf, fx, 13, 6, "efg ");
			keyflag[0] = 0;
		}
		if(keyflag[1] != 0 && fx < 37){//自机右移
			putstr(win, winbuf, fx, 13, 6, " efg");
			fx++;
			keyflag[1] = 0;
		}
		if(keyflag[2] != 0 && laserwait == 0){//炮弹飞行坐标(lx, ly)
			laserwait = 15;
			lx = fx + 1;
			ly = 13;
		}
		
		/*外星人移动*/
		if(movewait != 0)
			movewait--;
		else{
			movewait = movewait0;
			if(ix + idir > 14 || ix + idir < 0){//超出窗口范围，则改变左右移动方向并敌机前进一行
				if(iy + invline == 13)break;/*结束游戏*/
				idir = -idir;
				putstr(win, winbuf, ix, iy, 0,  "                          ");//与invstr0中有效字符对应
				iy++;
			}else{
				ix += idir;
			}
			for(i = 0; i < invline; i++)//重新绘制所有敌机
				putstr(win, winbuf, ix, iy + i, 2, invstr + i * 32);
		}
		
		/*炮弹处理 坐标(lx, ly) 敌机坐标(ix, iy)*/
		if(ly > 0){
			if(ly < 13){//炮弹是否飞到敌机区域内
				if(ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline)
					putstr(win, winbuf, ix, ly, 2, invstr + (ly - iy) * 32);
				else
					putstr(win, winbuf, lx, ly, 0, " ");
			}
			ly--;
			if(ly > 0)//绘制炮弹轨迹
				putstr(win, winbuf, lx, ly, 3, "h");
			else{
				point -= 10;
				if(point <= 0)point = 1;
			}
			if(ix < lx && lx < ix + 25 && iy <= ly && ly < iy + invline){//飞到敌机区域内
				p = invstr + (ly - iy) * 32 + (lx - ix);//炮弹对应的敌机区域位置
				if(*p != ' '){//有敌机则击毁
					score += point;
					point++;					
					setdec8(s, score);
					putstr(win, winbuf, 10, 0, 7, s);//更新分数
					if(high < score){
						high = score;
						putstr(win, winbuf, 27, 0, 7, s);//更新最高分
					}
					//更新击中的敌机
					for(p--; *p != ' '; p--);
					for(i = 1; i < 5; i++)p[i] = ' ';//一个敌机由4字节构成，即“abcd”
					putstr(win, winbuf, ix, ly, 2, invstr + (ly - iy) * 32);//重新绘制炮弹所在行的敌机
					for(; invline > 0; invline--){//更新现存敌机行数
						for(p = invstr + (invline - 1) * 32; *p != 0; p++){
							if(*p != ' ')goto alive;
						}
					}
					/*全部消灭*/
					movewait0 -= movewait0 / 3;
					goto next_group;
			alive:
					ly = 0;
				}
			}
		}
	}
	/* GAME OVER */
	putstr(win, winbuf, 15, 6, 1, "GAME OVER");
	wait(0, timer, keyflag);
	for (i = 1; i < 14; i++) {
		putstr(win, winbuf, 0, i, 0, "                ");
	}
	goto restart;
}
/*在winbuf中的坐标(x,y)所在行写入col色的字符串s*/
/*每行336像素，图像高度16像素*/
void putstr(int win, char *winbuf, int x, int y, int col, unsigned char *s){
	int c, x0, i;
	char *p, *q, t[2];
	x = x * 8 + 8;
	y = y * 16 + 29;
	x0 = x;
	i = strlen(s);
	api_boxfilwin(win, x, y, x + i * 8 - 1, y + 15, 0);
	q = winbuf + y * 336;/*每行336像素*/
	t[1] = 0;
	for(;;){
		c = *s;
		if(c == 0)break;
		if(c != ' '){
			if('a' <= c && c <='h'){
				p = charset + 16 * (c - 'a');
				q += x;
				for(i = 0; i < 16; i++){//绘制一个敌机
					if(p[i] & 0x80)q[0] = col;
					if(p[i] & 0x40)q[1] = col;
					if(p[i] & 0x20)q[2] = col;
					if(p[i] & 0x10)q[3] = col;
					if(p[i] & 0x08)q[4] = col;
					if(p[i] & 0x04)q[5] = col;
					if(p[i] & 0x02)q[6] = col;
					if(p[i] & 0x01)q[7] = col;
					q += 336;
				}
				q -= 336 * 16 + x;/*恢复原值*/
			}else{
				t[0] = *s;
				api_putstrwin(win, x, y, col, 1, t);
			}
		}
		s++;
		x += 8;
	}
	api_refreshwin(win, x0, y, x, y + 16);
	return;
}
/*等待函数，并将键盘数据写到keyflag中*/
void wait(int i, int timer, char *keyflag){
	int j;
	if(i > 0){/*等待一段时间*/
		api_settimer(timer, i);
		i = 128;
	}else{/*进入*/
		i = 0x0a;
	}
	for(;;){/*写入键盘数据*/
		j = api_getkey(1);
		if(i == j)break;
		if(j == '4')keyflag[0] = 1;/*left*/
		if(j == '6')keyflag[1] = 1;/*right*/
		if(j == ' ')keyflag[2] = 1;/*space*/
	}
	return;
}
/*将i用十进制表示并存入s*/
void setdec8(char *s, int i){
	int j;
	for(j = 7; j >= 0; j--){
		s[j] = '0' + i % 10;
		i /= 10;
	}
	s[8] = 0;
	return;
}
