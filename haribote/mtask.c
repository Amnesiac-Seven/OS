#include"bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

//返回现在活动中的struct TASK的内存地址
struct TASK *task_now(void){
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	return tl->tasks[tl->now];
}
//向struct TASKLEVEL中添加一个任务
void task_add(struct TASK *task){
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	tl->tasks[tl->running] = task;
	tl->running++;
	task->flags = 2;
	return;
}
//从struct TASKLEVEL中删除一个任务
void task_remove(struct TASK *task){
	int i;
	struct TASKLEVEL *tl = &taskctl->level[task->level];
	for(i = 0; i < tl->running; i++){/*寻找task所在的位置*/
		if(tl->tasks[i] == task)
			break;
	}
	tl->running--;
	if(i < tl->now)/*需要移动成员，要相应地处理 */
		tl->now--;
	if(tl->now >= tl->running)/*如果now的值出现异常，则进行修正*/
		tl->now = 0;
	task->flags = 1; /* 休眠中 */
	for(; i < tl->running; i++)
		tl->tasks[i] = tl->tasks[i + 1];
	return;
}
//在任务切换时决定接下来切换到哪个LEVEL
void task_switchsub(void){
	int i;
	/*寻找最上层的LEVEL */
	for(i = 0; i < MAX_TASKLEVELS; i++){
		if(taskctl->level[i].running > 0)
			break;
	}
	taskctl->now_lv = i;
	taskctl->lv_change = 0;
	return;
}
//闲置任务
void task_idle(void){
	for(;;)
		io_hlt();
}
//在memman中分配所有任务内存，并对任务进行初始化
struct TASK *task_init(struct MEMMAN *memman){
	int i;
	struct TASK *task, *idle;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR*)ADR_GDT;
	taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].tss.ldtr = (TASK_GDT0 + MAX_TASKS + i) * 8;
		set_segmdesc(gdt + TASK_GDT0 + i, 103, (int) &taskctl->tasks0[i].tss, AR_TSS32);
		set_segmdesc(gdt + TASK_GDT0 + MAX_TASKS + i, 15, (int) taskctl->tasks0[i].ldt, AR_LDT);
	}
	for(i = 0; i < MAX_TASKLEVELS; i++){
		taskctl->level[i].running = 0;
		taskctl->level[i].now = 0;
	}
	task = task_alloc();
	task->flags = 2;/*活动中标志*/
	task->priority = 2;
	task->level = 0;/*最高LEVEL */
	task_add(task);
	task_switchsub();/* LEVEL设置*/
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, task->priority);
	
	idle = task_alloc();
	idle->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024;
	idle->tss.eip = (int) &task_idle;
	idle->tss.es = 1 * 8;
	idle->tss.cs = 2 * 8;
	idle->tss.ss = 1 * 8;
	idle->tss.ds = 1 * 8;
	idle->tss.fs = 1 * 8;
	idle->tss.gs = 1 * 8;
	task_run(idle, MAX_TASKLEVELS - 1, 1);
	return task;
}
// 在taskctl中找到一个未被使用的任务，将其初始化并返回
struct TASK *task_alloc(void){
	int i;
	struct TASK *task;
	for(i = 0; i < MAX_TASKS; i++){
		if(taskctl->tasks0[i].flags == 0){
			task = &taskctl->tasks0[i];
			task->flags=1;/*正在使用的标志，休眠状态*/
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = 0; /*这里先置为0*/
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.iomap = 0x40000000;
			task->tss.ss0 = 0;
			return task;
		}
	}
	return 0; /*全部正在使用*/
}
//根据priority调整task的优先级并运行（将其添加至taskctl中，并更新running）
void task_run(struct TASK *task, int level, int priority){
	if(level < 0)/*不改变LEVEL */
		level = task->level;
	if(priority > 0)
		task->priority = priority;
	if(task->flags == 2 && task->level != level)/*改变活动中的LEVEL */
		task_remove(task); /*这里执行之后flag的值会变为1，于是下面的if语句块也会被执行*/
	if(task->flags != 2){/*从休眠状态唤醒的情形*/
		task->level = level;/*活动中标志*/
		task_add(task);
	}
	taskctl->lv_change = 1; /*下次任务切换时检查LEVEL */
	return;
}
//进程转换
void task_switch(void){
	struct TASKLEVEL *tl = &taskctl->level[taskctl->now_lv];
	struct TASK *new_task, *now_task = tl->tasks[tl->now];
	tl->now++;
	if(tl->now == tl->running)
		tl->now = 0;
	if(taskctl->lv_change != 0){/*修改任务等级*/
		task_switchsub();
		tl = &taskctl->level[taskctl->now_lv];
	}
	new_task = tl->tasks[tl->now];
	timer_settime(task_timer, new_task->priority);
	if(new_task != now_task) 
		farjmp(0, new_task->sel);
	return;
}
//task休眠：检查是否为当前正在运行的进程，如果是则还额外需要执行任务转换操作，否则需要考虑在当前运行的进程前面还是后面
void task_sleep(struct TASK *task){
	struct TASK *now_task;
	if(task->flags == 2){//处于唤醒状态
		now_task = task_now();
		task_remove(now_task);
		if(task == now_task){
			task_switchsub();
			now_task = task_now();
			farjmp(0, now_task->sel);
		}
	}
	return;
}
