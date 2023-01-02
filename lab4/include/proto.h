
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
void NormalA();
void ReaderB();
void ReaderC();
void ReaderD();
void WriterE();
void WriterF();

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

/* i8259.c */
PUBLIC void put_irq_handler(int irq, irq_handler handler);
PUBLIC void spurious_irq(int irq);

/* clock.c */
PUBLIC void clock_handler(int irq);


/* 以下是系统调用相关 */

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

