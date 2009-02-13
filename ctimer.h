#ifdef CTIMER_H
#error "multiple include"
#endif
#define CTIMER_H

#include <sys/types.h>

u_int64_t get_usecs(void);
