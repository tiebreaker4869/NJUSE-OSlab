
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

SEMAPHORE r; 
SEMAPHORE w; 
SEMAPHORE mutex;

SEMAPHORE r_mutex; int r_count;
SEMAPHORE w_mutex; int w_count;

/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	disp_str("-----\"kernel_main\" begins-----\n");
	
	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
	for (i = 0; i < NR_TASKS; i++) {
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid

		p_proc->ticks = 0;
		p_proc->delay_ticks = 0;
		p_proc->inqueue = 0;

		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs	= ((8 * 0) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ds	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.es	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.fs	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.ss	= ((8 * 1) & SA_RPL_MASK & SA_TI_MASK)
			| SA_TIL | RPL_TASK;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK)
			| RPL_TASK;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	k_reenter = 0;
	ticks = 0;

	p_proc_ready	= proc_table+NR_TASKS-1;

    // 初始化任务状态
    for (int i = 0; i < NR_TASKS - 1; i ++) {
        task_status[i] = 2;
    }

    print_index = 1;

	init_sema(&r, READERS);
	init_sema(&w, WRITERS);
	init_sema(&mutex, MUTEX);
	init_sema(&r_mutex, MUTEX); r_count = 0;
	init_sema(&w_mutex, MUTEX); w_count = 0;

	init_queue();
	init_clock();
	clear();

	restart();

	while(1){}
}

void business(int task_time){
	milli_delay(task_time / HZ * T);
}

void FAIR_R(char* s, int task_time){
    int current_index = s[0] - 'A';
	while (1) {
        task_status[current_index] = 0;
		P(&mutex);

			P(&r);

				P(&r_mutex);
				if(r_count==0) P(&w);
				r_count++;
				V(&r_mutex);

		V(&mutex);
            task_status[current_index] = 1;
			business(task_time);
			
				P(&r_mutex);
				r_count--;
				if(r_count==0) V(&w);
				V(&r_mutex);
			
			V(&r);
            task_status[current_index] = 2;

            mydelay(1 * RR / HZ * T);
	}
}
void FAIR_W(char* s, int task_time){
	int current_index = s[0] - 'A';
    while (1) {
        task_status[current_index] = 0;
		P(&mutex);

			P(&w);

		V(&mutex);
            task_status[current_index] = 1;
			business(task_time);
			V(&w);
            task_status[current_index] = 2;
            mydelay(1 * RR/ HZ * T)
	}
}

void READER_FIRST_R(char* s, int task_time){
	while (1) {	
		P(&r);
		
			P(&r_mutex);
			if(r_count==0) P(&w);
			r_count++;
			V(&r_mutex);
		
		business(task_time);
		
			P(&r_mutex);
			r_count--;
			if(r_count==0) V(&w);
			V(&r_mutex);
		
		V(&r);
		
		// avoid straving
		mydelay(task_time/HZ*T/2);
	}
}
void READER_FIRST_W(char* s, int task_time){
	while (1) {		
		P(&w);
		business(task_time);
		V(&w);
	}
}

void WRITER_FIRST_R(char* s, int task_time){
	while (1) {	
		P(&r);

			P(&mutex);
				P(&r_mutex);
				if(r_count==0) P(&w);
				r_count++;
				V(&r_mutex);
			V(&mutex);

		business(task_time);
		
				P(&r_mutex);
				r_count--;
				if(r_count==0) V(&w);
				V(&r_mutex);
		
		V(&r);
	}
}
void WRITER_FIRST_W(char* s, int task_time){
	while (1) {
		P(&w_mutex);
		if(w_count==0)	P(&mutex);
		w_count++;
		V(&w_mutex);

		P(&w);
		business(task_time);
		V(&w);

		P(&w_mutex);
		w_count--;
		if(w_count==0)	V(&mutex);
		V(&w_mutex);

		// avoid straving
		mydelay(task_time/HZ*T);
	}
}

void(* R_MODE[MODE_COUNT])(char*, int) = {FAIR_R, READER_FIRST_R, WRITER_FIRST_R};
void(* W_MODE[MODE_COUNT])(char*, int) = {FAIR_W, READER_FIRST_W, WRITER_FIRST_W};

/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	int tt = 2 * RR;
	R_MODE[MODE]("A", tt);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	int tt = 3 * RR;
	R_MODE[MODE]("B", tt);
}

/*======================================================================*
                               TestC
 *======================================================================*/
void TestC()
{
	int tt = 3 * RR;
	R_MODE[MODE]("C", tt);
}

/*======================================================================*
                               TestD
 *======================================================================*/
void TestD()
{
	int tt = 3 * RR;
	W_MODE[MODE]("D", tt);
}

/*======================================================================*
                               TestE
 *======================================================================*/
void TestE()
{
	int tt = 4 * RR;
	W_MODE[MODE]("E", tt);
}

/*======================================================================*
                               TestF
 *======================================================================*/

void TestF()
{
	int tt = 1 * RR;
    char* s;
    if (print_index > 20) {
        mydelay(tt / HZ * T);
    } else {
        if (print_index < 10) {
            char index[2] = {print_index + '0', '\0'};
            s = index;
        } else {
            char index[3] = {print_index / 10 + '0', print_index % 10 + '0', '\0'};
            s = index;
        }
    }

    print(s);

    print(" ");

    while (1) {
        for (int i = 0; i < NR_TASKS-1; i ++) {
            switch (task_status[i]) {
                case 0:
                    print("X");
                    print(" ");
                    break;
                case 1:
                    print("O");
                    print(" ");
                case 2:
                    print("Z");
                    print(" ");
            }
        }
        print("\n");
        mydelay(tt/HZ*T);
	}
}