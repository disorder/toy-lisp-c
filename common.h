#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
//#define raise(...) (printf(__VA_ARGS__), abort())
#define raise(...) (printf(__VA_ARGS__), DIE? abort() : longjmp(repl,1))
//#define debug(...) printf(__VA_ARGS__)
#define debug(...)
#define info(...) printf(__VA_ARGS__)

// types
#define u32 unsigned int
#define u8 unsigned char

// for REPL
#include <setjmp.h>
extern jmp_buf repl;
extern int DIE;

#endif
