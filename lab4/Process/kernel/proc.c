
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                               proc.c
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
                              schedule
 *======================================================================*/
PUBLIC void schedule()
{
	// my Round-Robin algorithm, use a queue to get next process
	while(QUEUE.head==QUEUE.tail) ;
	p_proc_ready = dequeue();
	p_proc_ready->ticks = 1;
	// sys_print(p_proc_ready->p_name);
}

PUBLIC void init_queue(){
	QUEUE.head = 0;
	QUEUE.tail = 0;
}

PUBLIC void enqueue(PROCESS* p){
	p->inqueue = 1;
	QUEUE.q[QUEUE.tail++%8] = p;
}

PUBLIC PROCESS* dequeue(){
	PROCESS* p = QUEUE.q[QUEUE.head++%8];
	p->inqueue = 0;
	return p;
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           sys_print
 *======================================================================*/
PUBLIC void sys_print(char* info)
{
	if (info[0] == 'X' && info[1] == '\0') {
        disp_color_str(info, BRIGHT | RED);
    } else if (info[0] == 'O' && info[1] == '\0') {
        disp_color_str(info, BRIGHT | GREEN);
    } else if (info[0] == 'Z' && info[1] == '\0') {
        disp_color_str(info, BRIGHT | BLUE);
    } else {
        disp_str(info);
    }
}

/*======================================================================*
                           sys_delay
 *======================================================================*/
PUBLIC void sys_delay(int milli_seconds)
{
	p_proc_ready->delay_ticks += milli_seconds * HZ / T;
	schedule();
}

/*======================================================================*
                           sys_P
 *======================================================================*/
PUBLIC void sleep(SEMAPHORE* s){
	p_proc_ready->delay_ticks = -1;
	s->queue[s->tail++%6] = p_proc_ready;
	schedule();
}

PUBLIC void volatile sys_P(SEMAPHORE* s){
	s->value--;
	if (s->value<0) sleep(s);
}

PUBLIC void init_sema(SEMAPHORE* s, int value){
	s->value = value;
	s->head = 0;
	s->tail = 0;
}

/*======================================================================*
                           sys_V
 *======================================================================*/
PUBLIC void wakeup(SEMAPHORE* s){
	s->queue[s->head++%6]->delay_ticks = 0;
	// from wait to ready, not to run, 
	// so do NOT let p_proc_ready = wakeup_process;
}

PUBLIC void volatile sys_V(SEMAPHORE* s){
	s->value++;
	if (s->value<=0) wakeup(s);
}