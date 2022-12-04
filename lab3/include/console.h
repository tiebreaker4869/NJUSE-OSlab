
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef _ORANGES_CONSOLE_H_
#define _ORANGES_CONSOLE_H_

#define SCR_UP	1	/* scroll forward */
#define SCR_DN	-1	/* scroll backward */

#define SCREEN_SIZE		(80 * 25)
#define SCREEN_WIDTH		80
#define TAB_WIDTH 4

#define DEFAULT_CHAR_COLOR	0x07	/* 0000 0111 黑底白字 */

typedef struct pos_stack {
	unsigned int esp;
	unsigned int pos[SCREEN_SIZE];
	unsigned int find_begin_pos; /*方便 find 模式退出清空*/
}STACK;

typedef struct redo_list { /* 方便进行撤销 */
	unsigned int size;
	char arr[SCREEN_SIZE];
	unsigned int find_begin_pos;
}REDO_LST;

/* CONSOLE */
typedef struct s_console
{
	unsigned int	current_start_addr;	/* 当前显示到了什么位置	  */
	unsigned int	original_addr;		/* 当前控制台对应显存位置 */
	unsigned int	v_mem_limit;		/* 当前控制台占的显存大小 */
	unsigned int	cursor;			/* 当前光标位置 */
	unsigned int find_begin_cursor; /*find mode 开始的时候的光标位置，方便 find 模式退出清空*/
	STACK backtrace_stack;          /* 用来回退的栈 */
	REDO_LST redo_lst;              /*用来撤销操作的 redo list*/
}CONSOLE;





#endif /* _ORANGES_CONSOLE_H_ */
