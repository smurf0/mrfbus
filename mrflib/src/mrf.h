#ifndef _MRF_INCLUDED_
#define _MRF_INCLUDED_

#include <mrf_sys.h>

//#include <mrf_types.h>
//#include <mrf_if.h>
#include <mrf_arch.h>
#include <mrf_debug.h>

//#include <mrf_sys_structs.h>



int mrf_init(void);

int mrf_main_loop(void);

int mrf_rtc_get(TIMEDATE *td);
int mrf_rtc_set(TIMEDATE *td);

int mrf_tick_enable();
int mrf_tick_disable();

int mrf_time(char *buff);

int mrf_app_init() __attribute__ ((constructor));
#endif
