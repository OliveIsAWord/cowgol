#ifndef COWGOL_CGEN_H
#define COWGOL_CGEN_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <endian.h>

#if BYTE_ORDER == BIG_ENDIAN
#error "Sorry, cowgol cgen doesn't work on big endian machines yet."
#endif

typedef uint8_t i1;
typedef uint16_t i2;
typedef uint32_t i4;
typedef uint64_t i8;

typedef int8_t s1;
typedef int16_t s2;
typedef int32_t s4;
typedef int64_t s8;

extern i8* __top;
extern i8* __himem;
extern i8* global_argv;

typedef union data data;
union data
{
	i8 i8;
	i4 i4[2];
	void* ptr;
};

#endif

