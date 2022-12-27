
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

	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS; p++) {
			if (p->ticks > greatest_ticks) {
				disp_str("<");
				disp_int(p->ticks);
				disp_str(">");
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		/* if (!greatest_ticks) { */
		/* 	for (p = proc_table; p < proc_table+NR_TASKS; p++) { */
		/* 		p->ticks = p->priority; */
		/* 	} */
		/* } */
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
                           自定义系统调用: sys_print
 *======================================================================*/

PUBLIC void sys_print(char* s) {
	int offset = p_proc_ready - proc_table;
	switch (offset)
	{
	case 0:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, RED));
		break;
	case 1:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, GREEN));
		break;
	case 2:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, BLUE));
		break;
	case 5:
		disp_str(s);
		break;
	case 4:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, PURPLE));
		break;
	case 3:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, YELLO));
		break;
	default:
		disp_str(s);
		break;
	}
}

/*======================================================================*
                           自定义系统调用: sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds) {
	p_proc_ready->wake = get_ticks() + (milli_seconds / (1000 / HZ));
	schedule();
}

/*======================================================================*
                           自定义系统调用: sys_P
 *======================================================================*/
PUBLIC void sys_P(void* mutex) {
	disable_irq(CLOCK_IRQ); // 关中断保证原语
	Semaphore* semaphore = (Semaphore*) mutex;
	semaphore->value --;

	if (semaphore->value < 0) {
		put_sleep(semaphore);
	}
	
	enable_irq(CLOCK_IRQ); // 一定要记得打开中断
}


/*======================================================================*
                           自定义系统调用: sys_V
 *======================================================================*/
PUBLIC void sys_V(void* mutex) {
    disable_irq(CLOCK_IRQ); // 关中断保证原语

    Semaphore* semaphore = (Semaphore*) mutex;
    semaphore->value ++;
    if (semaphore->value <= 0) {
        wake_up(semaphore);
    }

    enable_irq(CLOCK_IRQ); // 一定要记得打开中断
}

/*======================================================================*
                           自定义辅助方法: is_ready, 判定进程是否就绪
 *======================================================================*/

PUBLIC int is_ready(PROCESS* p) {
	// 睡醒时间片已达到 + 进程未完成 + 进程未阻塞
	if (p->wake <= get_ticks() && p->is_done == 0 && p->is_block == 0) {
		return 1;
	} else {
		return 0;
	}
}

/*======================================================================*
        自定义辅助方法: put_sleep, 把一个进程加入一个信号量的阻塞队列
 *======================================================================*/

PUBLIC void put_sleep(Semaphore* mutex) {
	mutex->wait_queue[-mutex->value - 1] = p_proc_ready;
	p_proc_ready->is_block = 1;
	schedule();
}

/*======================================================================*
        自定义辅助方法: wake_up, 唤醒一个等待的进程
 *======================================================================*/

PUBLIC void wake_up(Semaphore* mutex) {
    PROCESS* awake = mutex->wait_queue[0];
    int index = 0;
    // 选出优先级最高的进程
    for (int i = 0; i < (-mutex->value + 1); i ++) {
        if (mutex->wait_queue[i]->priority > awake->priority) {
            index = i;
            awake = mutex->wait_queue[i];
        }
    }

    // 空缺之后的部分迁移一格，填补空缺
    for (int i = index, i < -mutex->value; i ++) {
        mutex->wait_queue[i] = mutex->wait_queue[i + 1];
    }

    // 阻塞状态置为非阻塞
    awake->is_block = 0;
}