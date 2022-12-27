
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* EXTERN is defined as extern except in global.c */
#ifdef	GLOBAL_VARIABLES_HERE
#undef	EXTERN
#define	EXTERN
#endif

EXTERN	int		ticks;

EXTERN	int		disp_pos;
EXTERN	u8		gdt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	DESCRIPTOR	gdt[GDT_SIZE];
EXTERN	u8		idt_ptr[6];	// 0~15:Limit  16~47:Base
EXTERN	GATE		idt[IDT_SIZE];

EXTERN	u32		k_reenter;

EXTERN	TSS		tss;
EXTERN	PROCESS*	p_proc_ready;

extern	PROCESS		proc_table[];
extern	char		task_stack[];
extern  TASK            task_table[];
extern	irq_handler	irq_table[];

// 以下是新添加的
typedef struct semaphore {
    int value;
    PROCESS* wait_queue[NR_TASKS];
} Semaphore;

EXTERN Semaphore read_mutex; // value=并发数
EXTERN Semaphore write_mutex;// value=1
EXTERN Semaphore count_mutex;// value=1
EXTERN Semaphore write_mutex_mutex;// value=1

EXTERN int reader_limit;     // 允许同时读的个数
EXTERN int writer_limit;    // 允许同时写的个数，默认为1
EXTERN int read_prepared_count;
EXTERN int read_count;   // 正在读的个数
EXTERN int solve_hunger; // 是否解决饿死