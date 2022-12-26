
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
	PROCESS* p;
	int	 greatest_ticks = 0;

	while (!greatest_ticks) {
		for (p = proc_table; p < proc_table+NR_TASKS; p++) {
			if (p->ticks > greatest_ticks) {
				disp_str("<");
				disp_int(p->ticks);
				disp_str(">");
				greatest_ticks = p->ticks;
				p_proc_ready = p;
			}
		}

		/* if (!greatest_ticks) { */
		/* 	for (p = proc_table; p < proc_table+NR_TASKS; p++) { */
		/* 		p->ticks = p->priority; */
		/* 	} */
		/* } */
	}
}

/*======================================================================*
                           sys_get_ticks
 *======================================================================*/
PUBLIC int sys_get_ticks()
{
	return ticks;
}

/*======================================================================*
                           自定义系统调用: sys_print
 *======================================================================*/

PUBLIC void sys_print(char* s) {
	int offset = p_proc_ready - proc_table;
	switch (offset)
	{
	case 0:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, RED));
		break;
	case 1:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, GREEN));
		break;
	case 2:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, BLUE));
		break;
	case 5:
		disp_str(s);
		break;
	case 4:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, PURPLE));
		break;
	case 3:
		disp_color_str(s, BRIGHT | MAKE_COLOR(BLACK, YELLO));
		break;
	default:
		disp_str(s);
		break;
	}
}