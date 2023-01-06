
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
	PROCESS* p;
	int	 greatest_ticks = 0;
	int	 current_tick = get_ticks();

	while (1) {
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS) {
			p_proc_ready = proc_table;
		}
		if (p_proc_ready->isBlocked == FALSE && 
			p_proc_ready->wake_tick <= current_tick) {
			break; // 寻找到进程
		}
	}

	// for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	// 	if (p->wake_tick > 0) {
	// 		p->wake_tick--;
	// 	}
	// }

	// while (!greatest_ticks) {
	// 	for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	// 		if (p_proc_ready->wake_tick > current_tick || p->isBlocked == TRUE) {
	// 			continue;
	// 		}
	// 		if (p->ticks > greatest_ticks) {
	// 			greatest_ticks = p->ticks;
	// 			p_proc_ready = p;
	// 		}
	// 	}

	// 	if (!greatest_ticks) {
	// 		for (p = proc_table; p < proc_table + NR_TASKS; p++) {
	// 			if (p->ticks > 0) { // 说明被阻塞了
	// 				continue;
	// 			}
	// 			p->ticks = p->priority;
	// 		}
	// 	}
	// }
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC int sys_sleep(int milli_seconds) {
	p_proc_ready->wake_tick = get_ticks() + (milli_seconds / (1000 / HZ));
	// p_proc_ready->wake_tick = milli_seconds / (1000 / HZ);
	schedule();
}

/*======================================================================*
                           sys_print
 *======================================================================*/
PUBLIC int sys_print(char *str)
{
	if (disp_pos >= 80 * 25 * 2) {
		memset(0xB8000, 0, 80 * 25 * 2);
		disp_pos = 0;
	}

	if (*str == 'O') {
		disp_color_str(str, GREEN);
	} else if (*str == 'X') {
		disp_color_str(str, RED);
	} else if (*str == 'Z') {
		disp_color_str(str, BLUE);
	} else {
		disp_str(str);
	}
}

/*======================================================================*
                           sys_P
 *======================================================================*/
PUBLIC int sys_P(void *sem)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)sem;
	s->value--;
	if (s->value < 0) {
		// 将进程加入队列尾
		p_proc_ready->status = 0;
		p_proc_ready->isBlocked = TRUE;
		s->pQueue[s->tail] = p_proc_ready;
		s->tail = (s->tail + 1) % NR_TASKS;
		schedule();
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_V
 *======================================================================*/
PUBLIC int sys_V(void *sem)
{
	disable_irq(CLOCK_IRQ); // 保证原语
	SEMAPHORE *s = (SEMAPHORE *)sem;
	s->value++;
	if (s->value <= 0) {
		// 释放队列头的进程
		PROCESS *proc = s->pQueue[s->head];
		proc->status = 0;
		proc->isBlocked = FALSE;
		s->head = (s->head + 1) % NR_TASKS;
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           READER
 *======================================================================*/
PUBLIC void READER(int time_slice)
{
	switch (STRATEGY) {
	case 0:
		READER_rf(time_slice);
		break;
	case 1:
		READER_wf(time_slice);
		break;
	default:
		READER_fair(time_slice);
		break;
	}
}

/*======================================================================*
                           WRITER
 *======================================================================*/
PUBLIC void WRITER(int time_slice)
{
	switch (STRATEGY) {
	case 0:
		WRITER_rf(time_slice);
		break;
	case 1:
		WRITER_wf(time_slice);
		break;
	default:
		WRITER_fair(time_slice);
		break;
	}
}

/*======================================================================*
                           READER_rf
 *======================================================================*/
void READER_rf(int time_slice)
{
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	P(&readerLimit);
	p_proc_ready->status = 1; // 状态设置为正在读
	milli_delay(time_slice * TIME_SLICE);

	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_rf
 *======================================================================*/
void WRITER_rf(int time_slice)
{
	P(&writeBlock);

	p_proc_ready->status = 1; // 状态设置为正在写
	milli_delay(time_slice * TIME_SLICE);

	V(&writeBlock);
}

/*======================================================================*
                           READER_wf
 *======================================================================*/
void READER_wf(int time_slice)
{
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
	milli_delay(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_wf
 *======================================================================*/
void WRITER_wf(int time_slice)
{
	P(&mutex_writerNum);
	if (writerNum == 0)
		P(&readBlock); // 有写者，则禁止读
	writerNum++;
	V(&mutex_writerNum);

	// 开始写
	P(&writeBlock);

	p_proc_ready->status = 1;
	milli_delay(time_slice * TIME_SLICE);

	V(&writeBlock);
	// 完成写
	P(&mutex_writerNum);
	writerNum--;
	if (writerNum == 0)
		V(&readBlock); // 无写者，可读
	V(&mutex_writerNum);

}

/*======================================================================*
                           READER_fair
 *======================================================================*/
void READER_fair(int time_slice)
{
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
	milli_delay(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock);
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_fair
 *======================================================================*/
void WRITER_fair(int time_slice)
{

	P(&mutex_fair);
	P(&writeBlock);
	V(&mutex_fair);

	// 开始写
	p_proc_ready->status = 1;
	milli_delay(time_slice * TIME_SLICE);
	
	// 完成写
	V(&writeBlock);
}
