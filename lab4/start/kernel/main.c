
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
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


PUBLIC init_task() {
    proc_table[0].priority = 100;

    proc_table[1].priority = 90;

    proc_table[2].priority = 80;

    proc_table[3].priority = 95;

    proc_table[4].priority = 90;

    proc_table[5].priority = 80;

    proc_table[0].demand_time = 1;

    proc_table[1].demand_time = 2;

    proc_table[2].demand_time = 3;

    proc_table[3].demand_time = 3;

    proc_table[4].demand_time = 3;

    proc_table[5].demand_time = 4;

    proc_table[1].type = proc_table[2].type = proc_table[3].type = 'r';

    proc_table[4].type = proc_table[5].type = 'w';

    proc_table[0].type = 'o';

    /*初始化信号量相关*/
    reader_limit = 3;
    read_mutex.value = reader_limit;
    writer_limit = 1;
    write_mutex.value = writer_limit;
    count_mutex.value = 1;
    write_mutex_mutex.value = 1;

    // 是否需要解决饿死
    solve_hunger = 0;
}


/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main() {
    disp_str("-----\"kernel_main\" begins-----\n");

    TASK *p_task = task_table;
    PROCESS *p_proc = proc_table;
    char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
    u16 selector_ldt = SELECTOR_LDT_FIRST;
    int i;
    for (i = 0; i < NR_TASKS; i++) {
        strcpy(p_proc->p_name, p_task->name);    // name of the process
        p_proc->pid = i;            // pid

        p_proc->ldt_sel = selector_ldt;

        memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
               sizeof(DESCRIPTOR));
        p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
        memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
               sizeof(DESCRIPTOR));
        p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
        p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
                          | SA_TIL | RPL_TASK;
        p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
                          | SA_TIL | RPL_TASK;
        p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
                          | SA_TIL | RPL_TASK;
        p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
                          | SA_TIL | RPL_TASK;
        p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
                          | SA_TIL | RPL_TASK;
        p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK)
                          | RPL_TASK;

        p_proc->regs.eip = (u32) p_task->initial_eip;
        p_proc->regs.esp = (u32) p_task_stack;
        p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

        p_task_stack -= p_task->stacksize;
        p_proc++;
        p_task++;
        selector_ldt += 1 << 3;
    }


    k_reenter = 0;
    ticks = 0;

    p_proc_ready = proc_table;


    init_task();

    /* 初始化 8253 PIT */
    out_byte(TIMER_MODE, RATE_GENERATOR);
    out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
    out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

    put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
    enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */

    disp_pos = 0;
    for (i = 0; i < 80 * 25; i++) {
        disp_str(" ");
    }
    disp_pos = 0;

    restart();

    while (1) {}
}



/*======================================================================*
 *                      读者优先的读者进程
 *======================================================================*/

void reader_rf(char process) {
    while (1) {
        // 判断修改在读人数
        P(&count_mutex);
        if (read_prepared_count == 0) {
            P(&write_mutex);
        }
        read_prepared_count++;
        V(&count_mutex);

        P(&read_mutex);
        read_count++;

        for (int j = 0; j < p_proc_ready->demand_time; ++j) {
            if (j < p_proc_ready->demand_time - 1) {
                milli_delay(10);
            }
        }

        read_count--;
        V(&read_mutex);

        P(&count_mutex);
        read_prepared_count--;
        if (read_prepared_count == 0) {
            V(&write_mutex);
        }
        V(&count_mutex);

        p_proc_ready->is_done = solve_hunger;
        milli_delay(10); // 废弃当前时间片，至少等到下个时间片才能进入循环
    }
}


/*======================================================================*
 *                读者优先的写者进程
 *======================================================================*/

void writer_rf(char process) {
    while (1) {
        P(&write_mutex_mutex); // 只允许一个写者进程在writeMutex排队，其他写者进程只能在writeMutexMutex排队
        P(&write_mutex);
        for (int j = 0; j < p_proc_ready->demand_time; ++j) {
            if (j < p_proc_ready->demand_time - 1) {
                milli_delay(10);
            }
        }

        V(&write_mutex);
        V(&write_mutex_mutex);

        p_proc_ready->is_done = solve_hunger;
        milli_delay(10);
    }
}

/*======================================================================*
 *                A
 *======================================================================*/

void A() {
    while (1) {
        if (print_index > 20) {
            sleep(10);
        } else {
            disp_int(print_index);
            print(" ");
        }
        for (char process = 'B'; process <= 'F'; process++) {
            int index = process - 'A';
//            PROCESS *cur_process = proc_table + index;
//            if (cur_process->task_status == 0) {
//                print("X");
//                print(" ");
//            } else if (cur_process->task_status == 1) {
//                print("O");
//                print(" ");
//            } else if (cur_process->task_status == 2) {
//                print("Z");
//                print(" ");
//            }
            char pname[2] = {process, '\0'};
            print(pname);
        }

        print("\n");

        sleep(10);
    }
}

/*======================================================================*
 *                     B
 *======================================================================*/

void B() {
    reader_rf('B');
}

/*======================================================================*
 *                      C
 *======================================================================*/

void C() {
    reader_rf('C');
}

/*======================================================================*
 *                      D
 *======================================================================*/

void D() {
    reader_rf('D');
}

/*======================================================================*
 *                      E
 *======================================================================*/

void E() {
    writer_rf('E');
}

/*======================================================================*
 *                      F
 *======================================================================*/

void F() {
    writer_rf('F');
}