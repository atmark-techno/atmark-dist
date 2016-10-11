/*
 * $Id: stub_memaccount.c,v 1.3 2001/02/07 19:11:47 hno Exp $
 */

/* Stub function for programs not implementing statMemoryAccounted */
#include "config.h"
#include "util.h"
int
statMemoryAccounted(void)
{
    return -1;
}
