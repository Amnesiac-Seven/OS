#include"bootpack.h"

#define PIT_CTRL			0x0043
#define PIT_CNT0			0x0040
#define TIMER_FLAGS_ALLOC	1
#define TIMER_FLAGS_USING	2

struct TIMERCTL timerctl;

// 初始化所有计时器
void init_pit(void){
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count=0;
	timerctl.next=0xffffffff;
	timerctl.using=0;
	for(i=0;i<MAX_TIMER;i++)
		timerctl.timers0[i].flags=0;/* 未使用 */
	
	t=timer_alloc();
	t->flags=TIMER_FLAGS_USING;
	t->timeout=0xffffffff;
	t->next=0;
	timerctl.t0=t;//在链表中创建一个无穷大的节点，所以后续插入时只有两种可能：1.链表头2.链表中间
	timerctl.next=0xffffffff;
	
	return;
}
// 分配一个计时器
struct TIMER *timer_alloc(void){
	int i;
	for(i=0;i<MAX_TIMER;i++){
		if(timerctl.timers0[i].flags == 0){
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			timerctl.timers0[i].flags2 = 0;
			return &timerctl.timers0[i];
		}
	}
	return 0;
}

// 释放一个计时器
void timer_free(struct TIMER *timer){
	timer->flags=0;/* 未使用 */
	return;
}

// 初始化一个计时器
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
	timer->fifo=fifo;
	timer->data=data;
	return;
}

// 设置一个计时器，就是向数组timers中添加一个数据
// 由于该数组是按照timeout从小到大进行排序的，所以就需要先确认插入的位置
void timer_settime(struct TIMER *timer, unsigned int timeout){
	struct TIMER *t, *s;
	timer->timeout=timeout+timerctl.count;
	timer->flags=TIMER_FLAGS_USING;
	int e=io_load_eflags();
	io_cli();
	t=timerctl.t0;
	if(timer->timeout<=t->timeout){// 插在最前面，成为第一个中断计时器
		timerctl.t0=timer;
		timer->next=t;
		timerctl.next=timer->timeout;
		io_store_eflags(e);
		return;
	}
	for(;;){
		s=t;
		t=t->next;
		if(t==0)break;//放在链表最后
		if(t->timeout>=timer->timeout){// 插入位置在链表中间，所以需要确定相邻的两个结点(插入位置在两节点中间)
			s->next=timer;
			timer->next=t;
			io_store_eflags(e);
			return;
		}		
	}
	return;
}

//计时器中断，当timeout==0时则向timerctl.fifo中写入timerctl.data
void inthandler20(int *esp){
	char ts = 0;
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60); /* 把IRQ-00信号接收完了的信息通知给PIC */
	timerctl.count++;
	if(timerctl.next>timerctl.count)return;
	// 按照链表的做法，将未超时的计时器置为链表头
	timer=timerctl.t0;
	for(;;){
	/* timers的计时器全部在工作中，因此不用确认flags */
		if(timer->timeout>timerctl.count)//检查在时间点上有哪些计时器已过期，进行覆盖
			break;
		/* 超时 */
		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer != task_timer)// 当该计时器不是多任务计时器时，将data写入fifo中
			fifo32_put(timer->fifo, timer->data);
		else 
			ts = 1;
		timer = timer->next;/* 下一定时器的地址赋给timer */
	}
	timerctl.t0 = timer;/* 新移位 */
	timerctl.next = timerctl.t0->timeout;
	if(ts != 0)// 是多任务计时器时，切换任务
		task_switch();
	return;
}

int timer_cancel(struct TIMER *timer){
	int e;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();/*在设置过程中禁止改变定时器状态*/
	if(timer->flags == TIMER_FLAGS_USING){
		if(timerctl.t0 == timer){/*第一个定时器的取消处理*/
			t = timer->next;
			timerctl.t0 = t;
			timerctl.next = t->timeout;
		}else{
			/*非第一个定时器的取消处理*/
			/*找到timer前一个定时器*/
			t = timerctl.t0;
			for(;;){
				if(t->next == timer)
					break;
				t = t->next;
			}
		}
		t->next = timer->next;
		timer->flags = TIMER_FLAGS_ALLOC;
		io_store_eflags(e);
		return 1;
	}
	io_store_eflags(e);
	return 0; /*不需要取消处理*/
}

void timer_cancelall(struct FIFO32 *fifo){
	int e, i;
	struct TIMER *t;
	e = io_load_eflags();
	io_cli();
	for(i = 0; i < MAX_TIMER; i++){
		t = &timerctl.timers0[i];
		// 删除正在运行的、且由应用程序创建的计时器
		if(t->flags != 0 && t->flags2 != 0 && t->fifo == fifo){
			timer_cancel(t);
			timer_free(t);
		}
	}
	io_store_eflags(e);
	return;
}
