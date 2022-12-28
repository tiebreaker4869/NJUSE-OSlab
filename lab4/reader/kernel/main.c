
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

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");

	TASK *p_task = task_table;
	PROCESS *p_proc = proc_table;
	char *p_task_stack = task_stack + STACK_SIZE_TOTAL;
	u16 selector_ldt = SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++)
	{
		strcpy(p_proc->p_name, p_task->name); // name of the process
		p_proc->pid = i;					  // pid

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
			   sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	proc_table[0].type = proc_table[1].type = proc_table[2].type = 'r';
	proc_table[3].type = proc_table[4].type = 'w';


	proc_table[0].ticks = proc_table[0].needTime = 2;
	proc_table[1].ticks = proc_table[1].needTime = 3;
	proc_table[2].ticks = proc_table[2].needTime = 3;
	proc_table[3].ticks = proc_table[3].needTime = 3;
	proc_table[4].ticks = proc_table[4].needTime = 4;
	proc_table[5].ticks = proc_table[5].needTime = 1;

    for (int i = 0; i < NR_TASKS - 1; i ++) {
        task_status[i] = 2;
    }

	k_reenter = 0;
	ticks = 0;

	p_proc_ready = proc_table;

	/*初始化信号量相关*/
	readNum = 3;
	readMutex.value = readNum;
	writeNum = 1;
	writeMutex.value = writeNum;
	countMutex.value = 1;
	writeMutexMutex.value = 1;

	// 是否需要解决饿死
	solveHunger = 1;

    // 输出序号
    print_index = 1;

	/* 初始化 8253 PIT */
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8)(TIMER_FREQ / HZ));
	out_byte(TIMER0, (u8)((TIMER_FREQ / HZ) >> 8));

	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);					   /* 让8259A可以接收时钟中断 */
	disp_pos = 0;
	for (i = 0; i < 80 * 25; i++)
	{
		disp_str(" ");
	}
	disp_pos = 0;
	restart();

	while (1)
	{
	}
}

/*======================================================================*
                               A
 *======================================================================*/
void A()
{
	reader('A');
}

/*======================================================================*
                               B
 *======================================================================*/
void B()
{
	reader('B');
}

/*======================================================================*
                               C
 *======================================================================*/
void C()
{
	reader('C');
}
void D()
{
	writer('D');
}
void E()
{
	writer('E');
}
void F()
{
	while (1)
	{
        if (print_index > 20) {
            mysleep(TIME_SLICE);
            continue;
        }

        char* s;

        if (print_index < 10) {
            char index[2] = {'0' + print_index, '\0'};
            s = index;
        } else {
            char index[3] = {'0' + print_index / 10, '0' + print_index % 10, '\0'};
            s = index;
        }

        myprint(s);

        myprint(" ");

        for (int i = 0; i < NR_TASKS - 1; i ++) {
            switch (task_status[i]) {
                case 0:
                    // 正在读 / 写
                    myprint("O");
                    break;
                case 1:
                    // 等待读 / 写
                    myprint("X");
                    break;
                case 2:
                    // 休息
                    myprint("Z");
                    break;
                default:
                    break;
            }
            myprint(" ");
        }

        myprint("\n");

        print_index ++;

        mysleep(TIME_SLICE);
	}
}

void reader(char process)
{
    int current_index = process - 'A';
	while (1)
	{
        task_status[current_index] = 1;
		// 判断修改在读人数
		P(&countMutex);
		if (readPreparedCount == 0)
		{
			P(&writeMutex);
		}
		readPreparedCount++;
		V(&countMutex);

		P(&readMutex);
        task_status[current_index] = 0;
		readCount++;
		milli_delay(p_proc_ready->needTime * TIME_SLICE);
		readCount--;
		V(&readMutex);

		P(&countMutex);
		readPreparedCount--;
		if (readPreparedCount == 0)
		{
			V(&writeMutex);
		}
		V(&countMutex);

        if (solveHunger) {
            task_status[current_index] = 1;
        } else {
            task_status[current_index] = 2;
        }
        
		p_proc_ready->isDone = solveHunger;

		mysleep(TIME_SLICE); // rest
	}
}

void writer(char process)
{
    int current_index = process - 'A';
	while (1)
	{
        task_status[current_index] = 1;
		P(&writeMutexMutex); // 只允许一个写者进程在writeMutex排队，其他写者进程只能在writeMutexMutex排队
		P(&writeMutex);
        task_status[current_index] = 0;
		milli_delay(p_proc_ready->needTime * TIME_SLICE);
		V(&writeMutex);
		V(&writeMutexMutex);

        if (solveHunger) {
            task_status[current_index] = 1;
        } else {
            task_status[current_index] = 2;
        }

		p_proc_ready->isDone = solveHunger;

		mysleep(TIME_SLICE); //rest
	}
}