
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

/*======================================================================*
                           sys_sleep
 *======================================================================*/
PUBLIC int sys_sleep(int milli_seconds) {
	p_proc_ready->wake = get_ticks() + (milli_seconds / (1000 / HZ));
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
		p_proc_ready->is_blocked = TRUE;
		s->wait_queue[s->tail] = p_proc_ready;
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
		PROCESS *proc = s->wait_queue[s->head];
		proc->status = 0;
		proc->is_blocked = FALSE;
		s->head = (s->head + 1) % NR_TASKS;
	}
	enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           READER
 *======================================================================*/
PUBLIC void READER(int work_time)
{
	switch (STRATEGY) {
	case 0:
		READER_rf(work_time);
		break;
	case 1:
		READER_wf(work_time);
		break;
	default:
		READER_fair(work_time);
		break;
	}
}

/*======================================================================*
                           WRITER
 *======================================================================*/
PUBLIC void WRITER(int work_time)
{
	switch (STRATEGY) {
	case 0:
		WRITER_rf(work_time);
		break;
	case 1:
		WRITER_wf(work_time);
		break;
	default:
		WRITER_fair(work_time);
		break;
	}
}

/*======================================================================*
                           READER_rf
 *======================================================================*/
void READER_rf(int work_time)
{
	P(&m_reader_count);
	if (reader_count == 0)
		P(&writer_block); // 有读者，则禁止写
	reader_count ++;
	V(&m_reader_count);

	P(&reader_limit);
	p_proc_ready->status = 1; // 状态设置为正在读
	milli_delay(work_time * TIME_SLICE);

	P(&m_reader_count);
	reader_count --;
	if (reader_count == 0)
		V(&writer_block); // 无读者，可写
	V(&m_reader_count);

	V(&reader_limit);
}

/*======================================================================*
                           WRITER_rf
 *======================================================================*/
void WRITER_rf(int work_time)
{
	P(&writer_block);

	p_proc_ready->status = 1; // 状态设置为正在写
	milli_delay(work_time * TIME_SLICE);

	V(&writer_block);
}

/*======================================================================*
                           READER_wf
 *======================================================================*/
void READER_wf(int work_time)
{
	P(&reader_limit);

	P(&reader_block);

	P(&m_reader_count);
	if (reader_count == 0)
		P(&writer_block); // 有读者，则禁止写
	reader_count ++;
	V(&m_reader_count);

	V(&reader_block);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	milli_delay(work_time * TIME_SLICE);

	// 完成读
	P(&m_reader_count);
	reader_count --;
	if (reader_count == 0)
		V(&writer_block); // 无读者，可写
	V(&m_reader_count);

	V(&reader_limit);
}

/*======================================================================*
                           WRITER_wf
 *======================================================================*/
void WRITER_wf(int work_time)
{
	P(&m_writer_count);
	if (writer_count == 0)
		P(&reader_block); // 有写者，则禁止读
	writer_count ++;
	V(&m_writer_count);

	// 开始写
	P(&writer_block);

	p_proc_ready->status = 1;
	milli_delay(work_time * TIME_SLICE);

	V(&writer_block);
	// 完成写
	P(&m_writer_count);
	writer_count --;
	if (writer_count == 0)
		V(&reader_block); // 无写者，可读
	V(&m_writer_count);

}

/*======================================================================*
                           READER_fair
 *======================================================================*/
void READER_fair(int work_time)
{
	// 开始读
	P(&S);

	P(&reader_limit);
	P(&m_reader_count);
	if (reader_count == 0)
		P(&writer_block);
	V(&S);

	reader_count ++;
	V(&m_reader_count);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	milli_delay(work_time * TIME_SLICE);

	// 完成读
	P(&m_reader_count);
	reader_count --;
	if (reader_count == 0)
		V(&writer_block);
	V(&m_reader_count);

	V(&reader_limit);
}

/*======================================================================*
                           WRITER_fair
 *======================================================================*/
void WRITER_fair(int work_time)
{

	P(&S);
	P(&writer_block);
	V(&S);

	// 开始写
	p_proc_ready->status = 1;
	milli_delay(work_time * TIME_SLICE);
	
	// 完成写
	V(&writer_block);
}
