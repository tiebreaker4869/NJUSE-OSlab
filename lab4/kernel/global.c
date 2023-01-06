
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            global.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#define GLOBAL_VARIABLES_HERE

#include "type.h"
#include "const.h"
#include "protect.h"
#include "proto.h"
#include "proc.h"
#include "global.h"


PUBLIC	PROCESS			proc_table[NR_TASKS];

PUBLIC	char			task_stack[STACK_SIZE_TOTAL];

PUBLIC	TASK	task_table[NR_TASKS] = {{NormalA, STACK_SIZE_Normal_A, "NormalA"},
					                    {ReaderB, STACK_SIZE_Reader_B, "ReaderB"},
					                    {ReaderC, STACK_SIZE_Reader_C, "ReaderC"},
                                        {ReaderD, STACK_SIZE_Reader_D, "ReaderD"},
					                    {WriterE, STACK_SIZE_Writer_E, "WriterE"},
					                    {WriterF, STACK_SIZE_Writer_F, "WriterF"}};

PUBLIC	irq_handler		irq_table[NR_IRQ];

PUBLIC	system_call		sys_call_table[NR_SYS_CALL] = {sys_get_ticks,
                                                       sys_sleep,
                                                       sys_print,
                                                       sys_P,
                                                       sys_V};

PUBLIC SEMAPHORE reader_limit = {MAX_READERS, 0, 0};
PUBLIC SEMAPHORE writer_block = {1, 0, 0};
PUBLIC SEMAPHORE reader_block = {1, 0, 0};
PUBLIC SEMAPHORE m_reader_count = {1, 0, 0};
PUBLIC SEMAPHORE m_writer_count = {1, 0, 0};
PUBLIC SEMAPHORE S = {1, 0, 0};

PUBLIC int reader_count;

PUBLIC int writer_count;