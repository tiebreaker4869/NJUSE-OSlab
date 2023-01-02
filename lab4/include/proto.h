
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            proto.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/* klib.asm */
PUBLIC void	out_byte(u16 port, u8 value);
PUBLIC u8	in_byte(u16 port);
PUBLIC void	disp_str(char * info);
PUBLIC void	disp_color_str(char * info, int color);

/* protect.c */
PUBLIC void	init_prot();
PUBLIC u32	seg2phys(u16 seg);

/* klib.c */
PUBLIC void	delay(int time);

/* kernel.asm */
void restart();

/* main.c */
void PrinterA();
void ReaderB();
void ReaderC();
void ReaderD();
void WriterE();
void WriterF();

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */

/* proc.c */
PUBLIC  int     sys_get_ticks();        /* sys_call */
// 新添加的系统调用函数，系统调用会调用这些函数
PUBLIC void     sys_sleep(int);
PUBLIC void     sys_print(char*);
PUBLIC void     sys_P(void* semaphore);
PUBLIC void     sys_V(void* semaphore);

/* syscall.asm */
PUBLIC  void    sys_call();             /* int_handler */
PUBLIC  int     get_ticks();

// 新添加的封装好的系统调用
PUBLIC void     sleep(int);

PUBLIC void     print(char*);

PUBLIC void     P(void*);

PUBLIC void     V(void*);

// 读者和写者函数

PUBLIC void writer(int work_time);

PUBLIC void reader(int work_time);

// 读者优先

PUBLIC void reader_rf(int work_time);

PUBLIC void writer_rf(int work_time);

// 写者优先

PUBLIC void reader_wf(int work_time);

PUBLIC void writer_wf(int work_time);

// 读写公平

PUBLIC void reader_fair(int work_time);

PUBLIC void writer_fair(int work_time);

