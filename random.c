#include <stdlib.h>
#include <time.h>

#include "bce.h"

long g_rnd_seed = 1;

// from marcel's simple chess program
long getrandomnumber(void)
{
    long r = g_rnd_seed;
    
    r = 16807 * (r % 127773L) - 2836 * (r / 127773L);
    if (r < 0) r += 0x7fffffffL;
    
    return g_rnd_seed = r;
}
