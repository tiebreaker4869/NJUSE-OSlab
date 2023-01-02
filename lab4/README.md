# Lab4

## 读者写者问题

### 1. 功能要求

- 添加⼀个系统调用，其功能是接受⼀个 `int` 型参数 `milli_seconds` ，调用此系统调用的进程会在数 `milli_seconds` 毫秒内不被分配时间片
- 添加⼀个系统调用，其功能是接受⼀个 `char *` 型参数 `str` ，打印出 `str`
- 添加两个系统调用执行信号量PV操作，在此基础上模拟读者写者问题。普通进程A每个时间片输出每个 读者写者的状态，格式为： `[序号] [B] [C] [D] [E] [F]` ，如 `1 O O O X X` ，每个状态用对应的符号加上对应的颜色表示。为了方便检查，只输出20次(序号从1 ~ 20)
- 同时读的数量 $n$ 要求 $n$ = 1, 2, 3 均要实现，要求能够现场修改；读(写)完后休息的时间 $t$ ($t$ >= 0)可自定，每个进程休息时间可不同，要求能够现场修改
- 请分别实现读者优先和写者优先策略，要求能够现场修改
- 请想办法解决此问题中部分情况下的进程饿死问题(不能通过调整读写后的休息时长来解决，即便 $t$ = 0 时也要想办法解决)

### 2. 实现

> 基础代码使用 Oranges/chapter6/r
>
> 若代码与说明有误差，以代码为准

#### 2.1 添加进程

- main.c 添加进程体

  ```c
  void NormalA() {}
  void ReaderB() {}
  void ReaderC() {}
  void ReaderD() {}
  void WriterE() {}
  void WriterF() {}
  ```

- global.c  task_table 中添加相应的进程

  ```c
  PUBLIC	TASK	task_table[NR_TASKS] = {{NormalA, STACK_SIZE_Normal_A, "NormalA"},
  					                    {ReaderB, STACK_SIZE_Reader_B, "ReaderB"},
  					                    {ReaderC, STACK_SIZE_Reader_C, "ReaderC"},
                                          {ReaderD, STACK_SIZE_Reader_D, "ReaderD"},
  					                    {WriterE, STACK_SIZE_Writer_E, "WriterE"},
  					                    {WriterF, STACK_SIZE_Writer_F, "WriterF"}};
  ```

- proc.h 中修改 NR_TASKS 的值，并定义任务堆栈，修改 STACK_SIZE_TOTAL

  ```c
  /* Number of tasks */
  #define NR_TASKS	6
  
  /* stacks of tasks */
  #define STACK_SIZE_Normal_A	0x8000
  #define STACK_SIZE_Reader_B	0x8000
  #define STACK_SIZE_Reader_C	0x8000
  #define STACK_SIZE_Reader_D	0x8000
  #define STACK_SIZE_Writer_E	0x8000
  #define STACK_SIZE_Writer_F	0x8000
  
  #define STACK_SIZE_TOTAL	(STACK_SIZE_Normal_A + \
  							 STACK_SIZE_Reader_B + \
  							 STACK_SIZE_Reader_C + \
  							 STACK_SIZE_Reader_D + \
  							 STACK_SIZE_Writer_E + \
  							 STACK_SIZE_Writer_F)
  ```

- proto.h 添加任务执行体的函数声明

  ```c
  /* main.c */
  void NormalA();
  void ReaderB();
  void ReaderC();
  void ReaderD();
  void WriterE();
  void WriterF();
  ```

#### 2.2 添加系统调用

- proto.h 声明系统级和用户级系统调用函数

  ```c
  /* proc.c */
  PUBLIC  int     sys_get_ticks();        /* sys_call */
  PUBLIC  int     sys_sleep(int milli_seconds);
  PUBLIC  int     sys_print(char *str);
  PUBLIC  int     sys_P(void *sem);
  PUBLIC  int     sys_V(void *sem);
  
  /* syscall.asm */
  PUBLIC  void    sys_call();             /* int_handler */
  PUBLIC  int     get_ticks();
  
  PUBLIC  int     sleep(int milli_seconds);
  PUBLIC  int     print(char *str);
  PUBLIC  int     P(void *sem);
  PUBLIC  int     V(void *sem);
  ```

- const.h 修改 NR_SYS_CALL 的值

  ```c
  /* system call */
  #define NR_SYS_CALL     5
  ```

- global.c 添加相应的系统调用

  ```c
  PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                         sys_sleep,
                                                         sys_print,
                                                         sys_P,
                                                         sys_V};
  ```

- syscall.asm 中添加声明

  ```nasm
  _NR_get_ticks       equ 0 ; 要跟 global.c 中 sys_call_table 的定义相对应！
  INT_VECTOR_SYS_CALL equ 0x90
  
  _NR_sleep	    	equ 1
  _NR_print  			equ 2
  _NR_P      			equ 3
  _NR_V       		equ 4
  
  ; 导出符号
  global	get_ticks
  global 	sleep
  global 	print
  global 	P
  global 	V
  
  bits 32
  [section .text]
  
  ; ====================================================================
  ;                              get_ticks
  ; ====================================================================
  get_ticks:
  	mov	eax, _NR_get_ticks
  	int	INT_VECTOR_SYS_CALL
  	ret
  
  sleep:
  	mov eax, _NR_sleep
  	mov ebx, [esp + 4]
  	int INT_VECTOR_SYS_CALL
  	ret
  
  print:
  	mov eax, _NR_print
  	mov ebx, [esp + 4]
  	int INT_VECTOR_SYS_CALL
  	ret
  
  P:
  	mov eax, _NR_P
  	mov ebx, [esp + 4]
  	int INT_VECTOR_SYS_CALL
  	ret
  
  V:
  	mov eax, _NR_V
  	mov ebx, [esp + 4]
  	int INT_VECTOR_SYS_CALL
  	ret	
  ```

- 如果进行了参数传递，在 kernel.asm 中修改 sys_call 中的寄存器进栈出栈

  ```c
  sys_call:
      call	save
      sti
      push	ebx
      call	[sys_call_table + eax * 4]
      add		esp, 4
      mov 	[esi + EAXREG - P_STACKBASE], eax
      cli
      ret
  ```


##### sleep

> 接受⼀个 `int` 型参数 `milli_seconds` ，调用此系统调用的进程会在数 `milli_seconds` 毫秒内不被分配时间片

- 在 proc.h 中为每个 PROCESS 添加以下属性

  ```c
  int wake_tick;	/* 被唤醒时间 */
  int status;		/* 进程状态 0--等待读写  1--正在读写  2--休息*/
  int isBlocked; 	/* 是否被阻塞 */
  ```

- proc.c 中添加 sys_sleep() 系统调用处理函数体

  ```c
  PUBLIC int sys_sleep(int milli_seconds) {
  	p_proc_ready->wake_tick = get_ticks() + (milli_seconds / (1000 / HZ));
  	schedule();
  }
  ```

##### print

> 接受 `char*` 型参数 `str`，打印字符串

- const.h 中定义相应的颜色相关常量和宏函数

  ```c
  /* Color */
  /*
   * e.g. MAKE_COLOR(BLUE, RED)
   *      MAKE_COLOR(BLACK, RED) | BRIGHT
   *      MAKE_COLOR(BLACK, RED) | BRIGHT | FLASH
   */
  #define BLACK   0x0     /* 0000 */
  #define WHITE   0x7     /* 0111 */
  #define RED     0x4     /* 0100 */
  #define GREEN   0x2     /* 0010 */
  #define BLUE    0x1     /* 0001 */
  #define FLASH   0x80    /* 1000 0000 */
  #define BRIGHT  0x08    /* 0000 1000 */
  #define MAKE_COLOR(x,y) (x | y) /* MAKE_COLOR(Background,Foreground) */
  ```

- proc.c 中添加 sys_print() 系统调用处理函数体

  ```c
  PUBLIC int sys_print(char *str)
  {
  	if (disp_pos >= 80 * 25 * 2) {
  		memset(0xB8000, 0, 80 * 25 * 2);
  		disp_pos = 0;
  	}
  
  	if (*str == 'O') {
  		disp_color_str(str, GREEN);
  	} else if (*str == 'X') {
  		disp_color_str(str, RED);
  	} else if (*str == 'Z') {
  		disp_color_str(str, BLUE);
  	} else {
  		disp_str(str);
  	}
  }
  ```

##### PV

- proc.h 中定义 s_semaphore 结构体（信号量）

  ```c
  typedef struct s_semaphore
  {
  	int value;
  	int head;
  	int tail;
  	PROCESS * pQueue[NR_TASKS]; /* 等待信号量的进程队列 */
  } SEMAPHORE;
  ```

- proc.c 中添加 sys_P() 和 sys_V() 的系统调用处理函数体

  ```c
  /*======================================================================*
                             sys_P
   *======================================================================*/
  PUBLIC int sys_P(void *sem)
  {
  	disable_irq(CLOCK_IRQ); // 保证原语
  	SEMAPHORE *s = (SEMAPHORE *)sem;
  	s->value--;
  	if (s->value < 0) {
  		// 将进程加入队列尾
  		p_proc_ready->status = 0;
  		p_proc_ready->isBlocked = TRUE;
  		s->pQueue[s->tail] = p_proc_ready;
  		s->tail = (s->tail + 1) % NR_TASKS;
  		schedule();
  	}
  	enable_irq(CLOCK_IRQ);
  }
  
  /*======================================================================*
                             sys_V
   *======================================================================*/
  PUBLIC int sys_V(void *sem)
  {
  	disable_irq(CLOCK_IRQ); // 保证原语
  	SEMAPHORE *s = (SEMAPHORE *)sem;
  	s->value++;
  	if (s->value <= 0) {
  		// 释放队列头的进程
  		PROCESS *proc = s->pQueue[s->head];
  		proc->status = 0;
  		proc->isBlocked = FALSE;
  		s->head = (s->head + 1) % NR_TASKS;
  	}
  	enable_irq(CLOCK_IRQ);
  }
  ```

#### 2.3 进程调度

- const.h 中定义可以同时读的读者数量、调度策略和一个时间片长度

  ```c
  #define MAX_READERS     2   /* 可同时读的数量 1-3 */
  #define STRATEGY        0   /* 读优先--0  写优先--1  读写公平--2 */
  #define TIME_SLICE      1000 /* 一个时间片长度 */
  ```
  
- global.h 中定义各种信号量，以及记录读者写者数量的变量

  ```c
  EXTERN int readerNum;
  EXTERN int writerNum;
  
  EXTERN SEMAPHORE readerLimit;     // 同时读同一本书的人数
  EXTERN SEMAPHORE writeBlock;      // 限制写进程
  EXTERN SEMAPHORE readBlock;       // 限制读进程，保证写优先
  EXTERN SEMAPHORE mutex_readerNum; // 保护 readerNum 的变化
  EXTERN SEMAPHORE mutex_writerNum; // 保护 writerNum 的变化
  EXTERN SEMAPHORE mutex_fair;      // 实现读写公平
  ```

- global.c 中初始化信号量

  ```c
  PUBLIC SEMAPHORE readerLimit = {MAX_READERS, 0, 0};
  PUBLIC SEMAPHORE writeBlock = {1, 0, 0};
  PUBLIC SEMAPHORE readBlock = {1, 0, 0};
  PUBLIC SEMAPHORE mutex_readerNum = {1, 0, 0};
  PUBLIC SEMAPHORE mutex_writerNum = {1, 0, 0};
  PUBLIC SEMAPHORE mutex_fair = {1, 0, 0};
  ```

- main.c 中添加 cleanScreen() 方法，用于清屏和对变量的初始化

  ```c
  PUBLIC int kernel_main()
  {	...
  	cleanScreen(); // 清屏
  
  	restart();
  
  	while(1){}
  }
  
  // 添加清屏, 将显存指针指向第一个位置
  PUBLIC void cleanScreen() {
  	disp_pos = 0;
  	for (int i = 0; i < 80 * 25; i++) {
  		disp_str(" ");
  	}
  	disp_pos = 0;
  
  	// 初始化变量
  	readerNum = 0;
  	writerNum = 0;
  }
  ```

- kernel_main() 方法中，为进程初始化

  ```c
  for (int i = 0; i < NR_TASKS; i++) {
      proc_table[i].ticks = 1;
      proc_table[i].priority = 1;
      proc_table[i].wake_tick = 0;
      proc_table[i].status = 2;
      proc_table[i].isBlocked = 0;
  }
  ```

- 修改 proc.c 中的 schedule() 函数

  ```c
  PUBLIC void schedule()
  {
  	PROCESS* p;
  	int	 greatest_ticks = 0;
  	int	 current_tick = get_ticks();
  
  	while (1) {
  		p_proc_ready++;
  		if (p_proc_ready >= proc_table + NR_TASKS) {
  			p_proc_ready = proc_table;
  		}
  		if (p_proc_ready->isBlocked == FALSE && 
  			p_proc_ready->wake_tick <= current_tick) {
  			break; // 寻找到进程
  		}
  	}
  }
  ```

#### 2.4 读者写者

- proto.h 中声明总的读写接口函数（ WRITER() 和 READER() 方法），和三种策略各自的读写函数

  ```c
  /* 读写接口函数 */
  PUBLIC void WRITER(int time_slice);
  PUBLIC void READER(int time_slice);
  /* 读优先 */
  PUBLIC void WRITER_rf(int time_slice);
  PUBLIC void READER_rf(int time_slice);
  /* 写优先 */
  PUBLIC void WRITER_wf(int time_slice);
  PUBLIC void READER_wf(int time_slice);
  /* 读写公平 */
  PUBLIC void WRITER_fair(int time_slice);
  PUBLIC void READER_fair(int time_slice);
  ```

- main.c 中填写进程体的定义

  ```c
  void NormalA() {
  	milli_delay(200);
  	int n = 0;
  	while (TRUE) {
  		if (n++ < 20) {
  			if(n < 10) {
  				char tmp[4] = {n + '0', ' ', ' ', '\0'};
  				print(tmp);
  			} else {
  				char tmp[4] = {(n / 10) + '0', (n % 10) + '0', ' ', '\0'};
  				print(tmp);
  			}
  			for (int i = 1; i < NR_TASKS; i++) {
  				int status = proc_table[i].status;
  				if (status == 0) {
  					print("X ");
  				} else if (status == 1) {
  					print("O ");
  				} else if (status == 2) {
  					print("Z ");
  				}
  			}
  			print("\n");
  			milli_delay(TIME_SLICE);
  		}
  	}
  }
  
  void ReaderB() {
  	while (TRUE) {
          p_proc_ready->status = 0;
  		READER(2);
  		p_proc_ready->status = 2;
  		sleep(TIME_SLICE);
  	}
  }
  
  void ReaderC() {
  	while (TRUE) {
          p_proc_ready->status = 0;
  		READER(3);
  		p_proc_ready->status = 2;
  		sleep(TIME_SLICE);
  	}
  }
  
  void ReaderD() {
  	while (TRUE) {
          p_proc_ready->status = 0;
  		READER(3);
  		p_proc_ready->status = 2;
  		sleep(TIME_SLICE);
  	}
  }
  
  void WriterE() {
  	while (TRUE) {
          p_proc_ready->status = 0;
  		WRITER(3);
  		p_proc_ready->status = 2;
  		sleep(TIME_SLICE);
  	}
  }
  
  void WriterF() {
  	while (TRUE) {
          p_proc_ready->status = 0;
  		WRITER(4);
  		p_proc_ready->status = 2;
  		sleep(TIME_SLICE);
  	}
  }
  ```

- proc.c 中添加读者写者的PV操作

  ```c
  /*======================================================================*
                             READER
   *======================================================================*/
  PUBLIC void READER(int time_slice)
  {
  	switch (STRATEGY) {
  	case 0:
  		READER_rf(time_slice);
  		break;
  	case 1:
  		READER_wf(time_slice);
  		break;
  	default:
  		READER_fair(time_slice);
  		break;
  	}
  }
  
  /*======================================================================*
                             WRITER
   *======================================================================*/
  PUBLIC void WRITER(int time_slice)
  {
  	switch (STRATEGY) {
  	case 0:
  		WRITER_rf(time_slice);
  		break;
  	case 1:
  		WRITER_wf(time_slice);
  		break;
  	default:
  		WRITER_fair(time_slice);
  		break;
  	}
  }
  ```

##### 读者优先

```c
/*======================================================================*
                           READER_rf
 *======================================================================*/
void READER_rf(int time_slice)
{
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	P(&readerLimit);
	p_proc_ready->status = 1; // 状态设置为正在读
	sleep(time_slice * TIME_SLICE);

	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_rf
 *======================================================================*/
void WRITER_rf(int time_slice)
{
	P(&writeBlock);

	p_proc_ready->status = 1; // 状态设置为正在写
	sleep(time_slice * TIME_SLICE);

	V(&writeBlock);
}
```

##### 写者优先

```c
/*======================================================================*
                           READER_wf
 *======================================================================*/
void READER_wf(int time_slice)
{
	P(&readerLimit);

	P(&readBlock);

	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock); // 有读者，则禁止写
	readerNum++;
	V(&mutex_readerNum);

	V(&readBlock);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock); // 无读者，可写
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_wf
 *======================================================================*/
void WRITER_wf(int time_slice)
{
	P(&mutex_writerNum);
	writerNum++;
	if (writerNum == 1)
		P(&readBlock); // 有写者，则禁止读
	V(&mutex_writerNum);

	// 开始写
	P(&writeBlock);

	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	// 完成写
	P(&mutex_writerNum);
	writerNum--;
	if (writerNum == 0)
		V(&readBlock); // 无写者，可读
	V(&mutex_writerNum);

	V(&writeBlock);
}
```

##### 读写公平

```c
/*======================================================================*
                           READER_fair
 *======================================================================*/
void READER_fair(int time_slice)
{
	// 开始读
	P(&mutex_fair);

	P(&readerLimit);
	P(&mutex_readerNum);
	if (readerNum == 0)
		P(&writeBlock);
	V(&mutex_fair);

	readerNum++;
	V(&mutex_readerNum);

	// 进行读，对写操作加锁
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);

	// 完成读
	P(&mutex_readerNum);
	readerNum--;
	if (readerNum == 0)
		V(&writeBlock);
	V(&mutex_readerNum);

	V(&readerLimit);
}

/*======================================================================*
                           WRITER_fair
 *======================================================================*/
void WRITER_fair(int time_slice)
{

	P(&mutex_fair);
	P(&writeBlock);
	V(&mutex_fair);

	// 开始写
	p_proc_ready->status = 1;
	sleep(time_slice * TIME_SLICE);
	
	// 完成写
	V(&writeBlock);
}
```

