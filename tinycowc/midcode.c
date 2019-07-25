#include "globals.h"
#include "midcode.h"

#define MIDCODES_IMPLEMENTATION
#include "midcodes.h"

#define MIDBUFSIZ 16

static struct midcode midbuf[MIDBUFSIZ];
static int rdptr = 0;
static int wrptr = 0;

#define NEXT(ptr) ((ptr+1) % MIDBUFSIZ)
#define PREV(ptr) ((ptr-1) % MIDBUFSIZ)

static struct midcode* add_midcode(void)
{
    struct midcode* m = &midbuf[wrptr];
    if (wrptr == rdptr)
        fatal("midcode buffer overflow");

    wrptr = NEXT(wrptr);
    return m;
}

static void push_midend_state_machine(void)
{
}
