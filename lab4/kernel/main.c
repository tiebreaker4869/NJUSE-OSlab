
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
#define READING_TIME    10 * 50000 / HZ
// 写入时间
#define WRITING_TIME    10 * 50000 / HZ
// 每次任务间隔时间
#define GAP_TIME        50000 / HZ
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
	for (i = 0; i < NR_TASKS; i++) {
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
	p_proc_ready = proc_table;
	
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

void disp_one_line(int i, char *str, int color) {
	disp_color_str(names[i], color);
	disp_color_str(str, color);
	disp_str_sys("\n");
	disp_number++;
}

/*======================================================================*
                               reader_read 读者优先
 *======================================================================*/
void reader_read(int i, int time) {
	while (1) {
        status[i] = WAITING;
		p_sys(&mutex);
		if (reader_cnt == 0) {
			p_sys(&wrt);
		}
		reader_cnt++;
		v_sys(&mutex);
		
		p_sys(&reader);//最多允许多少个读者一起读
        status[i] = WORKING;
		milli_delay(time * READING_TIME);
        status[i] = RESTING;
		v_sys(&reader);
		
		p_sys(&mutex);
		reader_cnt--;
		if (reader_cnt == 0) {
			v_sys(&wrt);
		}
		v_sys(&mutex);

        delay_sys(GAP_TIME);
	}
}

/*======================================================================*
                               reader_write 写者优先
 *======================================================================*/
void reader_write(int i, int time) {
	while (1) {
		p_sys(&mutex3);
		p_sys(&r);
		p_sys(&mutex1);
		if (reader_cnt == 0) {
			p_sys(&w);
		}
		reader_cnt++;
		v_sys(&mutex1);
		v_sys(&r);
		v_sys(&mutex3);
		
		p_sys(&reader);//最多允许多少个读者一起读

		milli_delay(time * READING_TIME);
		
		v_sys(&reader);
		
		p_sys(&mutex1);
		reader_cnt--;
		if (reader_cnt == 0) {
			v_sys(&w);
		}
		v_sys(&mutex1);
		
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
		p_sys(&wrt);
        status[i] = WORKING;
		milli_delay(time * WRITING_TIME);
		status[i] = RESTING;
		v_sys(&wrt);
		delay_sys(GAP_TIME);
	}
}

/*======================================================================*
                               writer_write 写者优先
 *======================================================================*/
void writer_write(int i, int time) {
	while (1) {
		// 控制写进程
		//p_sys(&S);
		p_sys(&mutex2);
		writer_cnt++;
		if (writer_cnt == 1) {
			p_sys(&r);
		}
		v_sys(&mutex2);
		
		p_sys(&w);

		milli_delay(time * WRITING_TIME);

		v_sys(&w);
		
		p_sys(&mutex2);
		writer_cnt--;
		if (writer_cnt == 0) {
			v_sys(&r);
		}
		v_sys(&mutex2);
		//v_sys(&S);
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
            delay_sys(5000);
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

        disp_str_sys(s);

        disp_str_sys(" ");

        for (int i = 0; i < 5; i ++) {
            if (status[i] == RESTING) {
                disp_str_sys("Z");
            } else if (status[i] == WORKING) {
                disp_str_sys("O");
            } else if (status[i] == WAITING) {
                disp_str_sys("X");
            }
            disp_str_sys(" ");
        }

        disp_str_sys("\n");

		delay_sys(5000);
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