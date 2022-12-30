
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


// 进程调度
PUBLIC void schedule() {
	int t = 0;
	
	while (1) {
		t = get_ticks();
		p_proc_ready++;
		if (p_proc_ready >= proc_table + NR_TASKS) {
			p_proc_ready = proc_table;
		}
		if (p_proc_ready->waiting_semahore == 0 && p_proc_ready->ready_tick <= t) {
			break;
		}
	}
}

// 不分配时间片来切换，这部分被汇编调用
PUBLIC void sys_delay(int i) {
	p_proc_ready->ready_tick = get_ticks() + i / (1000 / HZ);
	// delay则进行进程切换
	schedule();
}

// 执行信号量P操作
PUBLIC void sys_p(SEMAPHORE *semaphore) {
	semaphore->number--;
	if (semaphore->number < 0) {
		// 等待一个信号量
		p_proc_ready->waiting_semahore = semaphore;
		
		semaphore->list[semaphore->end] = p_proc_ready;
		semaphore->end = (semaphore->end + 1) % SEMAPHORE_SIZE;
		// 进行进程调度
		schedule();
	}
}
// 执行信号量V操作
PUBLIC void sys_v(SEMAPHORE *semaphore) {
	semaphore->number++;
	if (semaphore->number <= 0) {
		// 等待队列中的第一个进程取出来
		PROCESS *p = semaphore->list[semaphore->start];
		p->waiting_semahore = 0;
		semaphore->start = (semaphore->start + 1) % SEMAPHORE_SIZE;
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks() {
	return ticks;
}


/*======================================================================*
                           sys_disp_str
 *======================================================================*/

PUBLIC void sys_disp_str(char* s) {
    if (s[0] == 'X' && s[1] == '\0') {
        disp_color_str(s, BRIGHT | RED);
    } else if (s[0] == 'O' && s[1] == '\0') {
        disp_color_str(s, BRIGHT | GREEN);
    } else if (s[0] == 'Z' && s[1] == '\0') {
        disp_color_str(s, BRIGHT | BLUE);
    } else {
        disp_str(s);
    }
	disp_str(s);
}