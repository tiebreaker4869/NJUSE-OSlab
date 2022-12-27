
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
PUBLIC void schedule() {
    disable_irq(CLOCK_IRQ);

    // 先检查一遍，如果都Done了，重新开始
    check_and_restart();
    // 新版
    PROCESS *select = proc_table;
    if (is_ready(select)) {
        //nothing
        p_proc_ready = select;
    } else {
        while (!is_ready(pre_proc)) {
            pre_proc++;
            if (pre_proc == (proc_table + 6)) {
                pre_proc = proc_table + 1;
            }
        }
        p_proc_ready = pre_proc;
        pre_proc++;
        if (pre_proc == proc_table + 6) {
            pre_proc = proc_table + 1;
        }

    }

    enable_irq(CLOCK_IRQ);
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks() {
    return ticks;
}

/*======================================================================*
                           新添加: sys_print
 *======================================================================*/

PUBLIC void sys_print(char *s) {
    disp_str(s);
}

/*======================================================================*
                           新添加: sys_sleep
 *======================================================================*/
PUBLIC void sys_sleep(int milli_seconds) {
    p_proc_ready->wake = get_ticks() + (milli_seconds / (1000 / HZ));
    schedule();
}

/*======================================================================*
                           新添加: sys_P
 *======================================================================*/
PUBLIC void sys_P(void *mutex) {
    disable_irq(CLOCK_IRQ); // 关中断保证原语
    Semaphore *semaphore = (Semaphore *) mutex;
    semaphore->value--;

    if (semaphore->value < 0) {
        put_sleep(semaphore);
    }

    enable_irq(CLOCK_IRQ); // 一定要记得打开中断
}


/*======================================================================*
                           新添加: sys_V
 *======================================================================*/
PUBLIC void sys_V(void *mutex) {
    disable_irq(CLOCK_IRQ); // 关中断保证原语

    Semaphore *semaphore = (Semaphore *) mutex;
    semaphore->value++;
    if (semaphore->value <= 0) {
        wake_up(semaphore);
    }

    enable_irq(CLOCK_IRQ); // 一定要记得打开中断
}

/*======================================================================*
                           自定义辅助方法: is_ready, 判定进程是否就绪
 *======================================================================*/

PUBLIC int is_ready(PROCESS *p) {
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

PUBLIC void put_sleep(Semaphore *mutex) {
    mutex->wait_queue[-mutex->value - 1] = p_proc_ready;
    p_proc_ready->is_block = 1;
    schedule();
}

/*======================================================================*
        自定义辅助方法: wake_up, 唤醒一个等待的进程
 *======================================================================*/

PUBLIC void wake_up(Semaphore *mutex) {
    PROCESS *awake = mutex->wait_queue[0];
    int index = 0;
    // 选出优先级最高的进程
    for (int i = 0; i < (-mutex->value + 1); i++) {
        if (mutex->wait_queue[i]->priority > awake->priority) {
            index = i;
            awake = mutex->wait_queue[i];
        }
    }

    // 空缺之后的部分迁移一格，填补空缺
    for (int i = index; i< -mutex->value; i++) {
        mutex->wait_queue[i] = mutex->wait_queue[i + 1];
    }

    // 阻塞状态置为非阻塞
    awake->is_block = 0;
}

/*======================================================================*
        自定义辅助方法: check_and_restart,检查并重启所有任务
 *======================================================================*/

PUBLIC void check_and_restart() {
    PROCESS *p;
    int all_done = 1;
    for (p = proc_table + 1; p < proc_table + NR_TASKS; p++){
        if(p->is_done==0){
            all_done = 0;
            return;
        }
    }
    if(all_done == 1){
        //如果全部做完了，任务重启
        for (p = proc_table + 1; p < proc_table + NR_TASKS; p++){
            p->is_done = 0;
        }
    }
}