
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/


typedef struct s_stackframe {	/* proc_ptr points here				↑ Low			*/
	u32	gs;		/* ┓						│			*/
	u32	fs;		/* ┃						│			*/
	u32	es;		/* ┃						│			*/
	u32	ds;		/* ┃						│			*/
	u32	edi;		/* ┃						│			*/
	u32	esi;		/* ┣ pushed by save()				│			*/
	u32	ebp;		/* ┃						│			*/
	u32	kernel_esp;	/* <- 'popad' will ignore it			│			*/
	u32	ebx;		/* ┃						↑栈从高地址往低地址增长*/		
	u32	edx;		/* ┃						│			*/
	u32	ecx;		/* ┃						│			*/
	u32	eax;		/* ┛						│			*/
	u32	retaddr;	/* return address for assembly code save()	│			*/
	u32	eip;		/*  ┓						│			*/
	u32	cs;		/*  ┃						│			*/
	u32	eflags;		/*  ┣ these are pushed by CPU during interrupt	│			*/
	u32	esp;		/*  ┃						│			*/
	u32	ss;		/*  ┛						┷High			*/
}STACK_FRAME;


typedef struct s_proc {
	STACK_FRAME regs;          /* process registers saved in stack frame */

	u16 ldt_sel;               /* gdt selector giving ldt base and limit */
	DESCRIPTOR ldts[LDT_SIZE]; /* local descriptors for code and data */

        int ticks;                 /* remained ticks */
        int priority;
		int wake; 					/*睡醒的时刻*/
		int demand_time;			/*需要的时间片*/
		int is_done;				/*是否结束*/
		int is_block;				/*是否阻塞*/
		char type;					/*读者进程还是写者进程, 读者进程: r, 写者进程: w, 输出进程: o*/
        int task_status;            /*正在进行的任务的状态, 正在等待: 0, 正在进行: 1, 休息: 2*/

	u32 pid;                   /* process id passed in from MM */
	char p_name[16];           /* name of the process */
}PROCESS;

typedef struct s_task {
	task_f	initial_eip;
	int	stacksize;
	char	name[32];
}TASK;


/* Number of tasks */
#define NR_TASKS	6

/* stacks of tasks */
#define STACK_SIZE_A	0x8000
#define STACK_SIZE_B	0x8000
#define STACK_SIZE_C	0x8000
#define STACK_SIZE_D	0x8000
#define STACK_SIZE_E	0x8000
#define STACK_SIZE_F	0x8000

#define STACK_SIZE_TOTAL	(STACK_SIZE_A + \
				STACK_SIZE_B + \
				STACK_SIZE_C + \
                STACK_SIZE_D + \
                STACK_SIZE_E + \
                STACK_SIZE_F)

