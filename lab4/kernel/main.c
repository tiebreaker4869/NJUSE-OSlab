
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

// 阅读者上限
#define READER_MAX        3
// 阅读时间
#define READING_TIME    10
// 写入时间
#define WRITING_TIME    10
// 每次任务间隔时间
#define GAP_TIME        10
// 读者优先还是写者优先 0是读者优先 1是写者优先
#define WHO_FIRST        0

#define WAITING 0

#define WORKING 1

#define RESTING 2

int reader_cnt = 0;
int writer_cnt = 0;
// 绿色，蓝色，红色，黄色
int colors[4] = {0x0A, 0x03, 0x0C, 0x0E};
char *names[5] = {"ReaderA", "ReaderB", "ReaderC", "WriterD", "WriterE"};
int status[5];
int print_index = 1;
SEMAPHORE mutex, wrt, mutex1, mutex2, mutex3, w, r, reader;
//SEMAPHORE reader, writer, writeBlock, S, mutexw;

void initSemaphore(SEMAPHORE *semaphore, int number);

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
	/* 初始化进程表 */
	for (i = 0; i < 6; i++) {
		strcpy(p_proc->p_name, p_task->name);    // name of the process
		p_proc->pid = i;            // pid
		
		p_proc->ldt_sel = selector_ldt;
		
		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3], sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | PRIVILEGE_TASK << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3], sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | PRIVILEGE_TASK << 5;
		p_proc->regs.cs = ((8 * 0) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ds = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.es = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.fs = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.ss = ((8 * 1) & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | RPL_TASK;
		p_proc->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | RPL_TASK;
		
		p_proc->regs.eip = (u32) p_task->initial_eip;
		p_proc->regs.esp = (u32) p_task_stack;
		p_proc->regs.eflags = 0x1202; /* IF=1, IOPL=1 */
		
		p_proc->ready_tick = 0;
		p_proc->waiting_semahore = 0;
		
		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}
	
	// 避免反复嵌套循环无法跳出
	k_reenter = 0;
	// 初始化时间
	ticks = 0;
	// 初始化显示行
	disp_number = 0;
	
	// 读者数量
	initSemaphore(&reader, READER_MAX);
	// 读者优先
	initSemaphore(&mutex, 1);
	// 读者优先
	initSemaphore(&wrt, 1);
	// 写者
	initSemaphore(&mutex1, 1);
	// 用于写者优先
	initSemaphore(&mutex2, 1);
	
	initSemaphore(&mutex3, 1);
	
	initSemaphore(&w, 1);
	
	initSemaphore(&r, 1);
	/*// 读者
	initSemaphore(&reader, READER_MAX);
	// 写者
	initSemaphore(&writer, 1);
	// 对writer_cnt的互斥信号量
	initSemaphore(&mutexw, 1);
	// 是否允许写的信号量
	initSemaphore(&writeBlock, 1);
	// 用于解决饿死问题的信号量
	initSemaphore(&S, 1);*/

    for (int i = 0; i < 5; i ++) {
        status[i] = RESTING;
    }
	
	reader_cnt = 0;
	writer_cnt = 0;
	
	// 清屏
	clear();
	disp_pos = 0;
	
	/* 扳机 */
	p_proc_ready = proc_table + 4;

	
	// 8253 PIT
	out_byte(TIMER_MODE, RATE_GENERATOR);
	out_byte(TIMER0, (u8) (TIMER_FREQ) / HZ);
	out_byte(TIMER0, (u8) ((TIMER_FREQ) / HZ >> 8));
	
	put_irq_handler(CLOCK_IRQ, clock_handler); /* 设定时钟中断处理程序 */
	enable_irq(CLOCK_IRQ);                     /* 让8259A可以接收时钟中断 */
	
	// 这个是我们操作系统启动时的第一个进程时的入口
	restart();
	
	while (1) {}
}

void initSemaphore(SEMAPHORE *semaphore, int number) {
	semaphore->number = number;
	semaphore->start = 0;
	semaphore->end = 0;
}

/*======================================================================*
                               reader_read 读者优先
 *======================================================================*/
void reader_read(int i, int time) {
	// char* s;
	// char name[2];
	// name[0] = 'A' + i;
	// name[1] = '\0';
	// s = name;
	while (1) {
        status[i] = WAITING;
		

		P(&mutex);
		if (reader_cnt == 0) {
			P(&wrt);
		}
		reader_cnt++;
		V(&mutex);
		
		P(&reader);//最多允许多少个读者一起读
        status[i] = WORKING;


		milli_delay(time * READING_TIME);

		V(&reader);
		
		P(&mutex);
		reader_cnt--;
		if (reader_cnt == 0) {
			V(&wrt);
		}
		V(&mutex);


		status[i] = RESTING;

		mysleep(10);
	}
}

/*======================================================================*
                               reader_write 写者优先
 *======================================================================*/
void reader_write(int i, int time) {
	while (1) {
		P(&mutex3);
		P(&r);
		P(&mutex1);
		if (reader_cnt == 0) {
			P(&w);
		}
		reader_cnt++;
		V(&mutex1);
		V(&r);
		V(&mutex3);
		
		P(&reader);//最多允许多少个读者一起读

		milli_delay(time * READING_TIME);
		
		V(&reader);
		
		P(&mutex1);
		reader_cnt--;
		if (reader_cnt == 0) {
			V(&w);
		}
		V(&mutex1);
		
		milli_delay(GAP_TIME);
	}
}

/*======================================================================*
                               ReaderA
 *======================================================================*/
void ReaderA() {
	if (WHO_FIRST) {
		reader_write(0, 2);
	} else {
		reader_read(0, 2);
	}
}

/*======================================================================*
                               ReaderB
 *======================================================================*/
void ReaderB() {
	if (WHO_FIRST) {
		reader_write(1, 3);
	} else {
		reader_read(1, 3);
	}
}

/*======================================================================*
                               ReaderC
 *======================================================================*/
void ReaderC() {
	if (WHO_FIRST) {
		reader_write(2, 3);
	} else {
		reader_read(2, 3);
	}
}

/*======================================================================*
                               writer_read 读者优先
 *======================================================================*/
void writer_read(int i, int time) {
	while (1) {
        status[i] = WAITING;
		P(&wrt);
        status[i] = WORKING;
		milli_delay(time * WRITING_TIME);
		V(&wrt);
		status[i] = RESTING;
		mysleep(GAP_TIME);
	}
}

/*======================================================================*
                               writer_write 写者优先
 *======================================================================*/
void writer_write(int i, int time) {
	while (1) {
		// 控制写进程
		//P(&S);
		P(&mutex2);
		writer_cnt++;
		if (writer_cnt == 1) {
			P(&r);
		}
		V(&mutex2);
		
		P(&w);

		milli_delay(time * WRITING_TIME);

		V(&w);
		
		P(&mutex2);
		writer_cnt--;
		if (writer_cnt == 0) {
			V(&r);
		}
		V(&mutex2);
		//V(&S);
		milli_delay(GAP_TIME);
	}
}


/*======================================================================*
                               WriterD
 *======================================================================*/
void WriterD() {
	if (WHO_FIRST) {
		writer_write(3, 3);
	} else {
		writer_read(3, 3);
	}
}

/*======================================================================*
                               WriterE
 *======================================================================*/
void WriterE() {
	if (WHO_FIRST) {
		writer_write(4, 4);
	} else {
		writer_read(4, 4);
	}
}

/*======================================================================*
                               F 普通进程
 *======================================================================*/
void F() {
	while (1) {
        if (print_index > 20) {
            mysleep(10);
            continue;
        }
        char* s;
        if (print_index < 10) {
            char i[2] = {print_index + '0', '\0'};
            s = i;
        } else {
            char i[3] = {print_index / 10 + '0', print_index % 10 + '0', '\0'};
            s = i;
        }

        myprint(s);

        myprint(" ");

        for (int i = 0; i < 5; i ++) {
            if (status[i] == RESTING) {
                myprint("Z");
            } else if (status[i] == WORKING) {
                myprint("O");
            } else if (status[i] == WAITING) {
                myprint("X");
            }
            myprint(" ");
        }

        myprint("\n");

        print_index ++;

		mysleep(10);
	}
}

void clear() {
	u8 *base = (u8 *) V_MEM_BASE;
	for (int i = 0; i < V_MEM_SIZE; i += 2) {
		base[i] = ' ';
		base[i + 1] = DEFAULT_CHAR_COLOR;
	}
	disp_pos = 0;
	disp_number = 0;
}