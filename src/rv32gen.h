//-------------------------------------------------------------------------------------------------
//                                                                           
//  GENRISCV - Risc-V IM Random Instruction Generator                                             
//  Copyright (C) 2021-2022 HT-LAB                                           
//                                                                                                          
//                                                                           
//-------------------------------------------------------------------------------------------------
//  https://github.com/htminuslab                                                     
//-------------------------------------------------------------------------------------------------
//
//  Revision History:  
//                                                                           
//  Date:          Revision         Author    
//  12-12-21       Created			Hans
//-------------------------------------------------------------------------------------------------
#pragma once

#define  MAJOR_VERSION 	0
#define  MINOR_VERSION 	02

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

#if defined(_MSC_VER)
	#include <time.h>
	#include <sys/types.h>
#else
	#include <stdint.h> 
	#include <unistd.h>	
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/stat.h>
#endif

#define NOEXCL				32										// Use to exclude destination register, 0..31, 32 means no exclude

#define MAXARGSTR			1280
#define MAX_WORD			120
#define MAX_LINE			256
#define MAX_SYMBOLS         1024									// Used for extracting symbols

// Quiet (-q) controlled print statement
#if defined(_MSC_VER)
	#define qprintf(fmt, ...)  do { if (!quiet)  fprintf(stdout, fmt, __VA_ARGS__); } while (0)	
#else
	#define qprintf(fmt, ARGS...)  do { if (!quiet)  fprintf(stdout, fmt, ##ARGS); } while (0)
#endif

char *gen_nonbranch(char *buf,int excl_reg);
uint32_t random(uint32_t mul);
void usage_exit(void);