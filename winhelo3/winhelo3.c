#include "apilib.h"

void HariMain(void)
{
	char buf[150*50];
	int win;

	api_initmalloc();
	win = api_openwin(buf, 150, 50, -1, "hello");
	api_boxfilwin(win,  8, 36, 141, 43, 6 /* 浅亮蓝 */);
	api_putstrwin(win, 28, 28, 0 /* 黑色 */, 12, "hello, world");
	for (;;) { 
		if (api_getkey(1) == 0x0a) {
			break; /*按下回车键则break; */
		}
	}
	api_end();
}
