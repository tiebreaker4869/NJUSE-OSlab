
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "string.h"
#include "proc.h"
#include "global.h"

/*======================================================================*
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
		int	 current_tick = get_ticks();

		while (1) {
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS) {
			p_proc_ready = proc_table;
		}
		if (p_proc_ready->is_blocked == FALSE && 
			p_proc_ready->wake <= current_tick) {
			break; // 寻找到进程
		}
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

PUBLIC void sys_sleep(int milli_seconds) {
	p_proc_ready->wake = get_ticks() + (milli_seconds / (1000 / HZ));

	schedule();
}

PUBLIC void sys_print(char* s) {

	if (disp_pos >= 80 * 25 * 2) {
		memset(0xB8000, 0, 80 * 25 * 2);
		disp_pos = 0;
	}

	if(s[0] == 'X') {
		disp_color_str(s, BRIGHT | RED);
	} else if (s[0] == 'O') {
		disp_color_str(s, BRIGHT | GREEN);
	} else if (s[0] == 'Z') {
		disp_color_str(s, BRIGHT | BLUE);
	} else {
		disp_str(s);
	}
}

PUBLIC void sys_P(void* semaphore) {
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)semaphore;
	s->value--;
	if (s->value < 0) {
		// 将进程加入队列尾
		p_proc_ready->status = 0;
		p_proc_ready->is_blocked = TRUE;
		s->queue[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % NR_TASKS;

		schedule();
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void sys_V(void* semaphore) {
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)semaphore;
	s->value++;
	if (s->value <= 0) {
		// 释放队列头的进程
		PROCESS *proc = s->queue[s->head];
		proc->status = 0;
		proc->is_blocked = FALSE;
		s->head = (s->head + 1) % NR_TASKS;
	}
	enable_irq(CLOCK_IRQ);
}

PUBLIC void reader_rf(int work_time) {
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	P(&readerLimit);
	p_proc_ready->status = 1; // 状态设置为正在读
	sleep(work_time * TIME_SLICE);

	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}


PUBLIC void writer_rf(int work_time) {
	P(&writeBlock);

	p_proc_ready->status = 1; // 状态设置为正在写
	sleep(work_time * TIME_SLICE);

	V(&writeBlock);
}

PUBLIC void reader_wf(int work_time) {
	P(&readerLimit);

	P(&readBlock);

	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	V(&readBlock);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(work_time * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

PUBLIC void writer_wf(int work_time) {
	P(&mutex_writerNum);
	writerNum++;
	if (writerNum == 1)
		P(&readBlock); // 有写者，则禁止读
	V(&mutex_writerNum);

	// 开始写
	P(&writeBlock);

	p_proc_ready->status = 1;
	sleep(work_time * TIME_SLICE);

	// 完成写
	P(&mutex_writerNum);
	writerNum--;
	if (writerNum == 0)
		V(&readBlock); // 无写者，可读
	V(&mutex_writerNum);

	V(&writeBlock);
}

PUBLIC void reader_fair(int work_time) {
	// 开始读
	P(&mutex_fair);

	P(&readerLimit);
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock);
	V(&mutex_fair);

	readerNum++;
	V(&mutex_readerNum);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(work_time * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock);
	V(&mutex_readerNum);

	V(&readerLimit);
}

PUBLIC void writer_fair(int work_time) {
	P(&mutex_fair);
	P(&writeBlock);
	V(&mutex_fair);

	// 开始写
	p_proc_ready->status = 1;
	sleep(work_time * TIME_SLICE);
	
	// 完成写
	V(&writeBlock);
}