/* Standard kernel/drivers logging library

    Copyright (C) 2016 Arthur M */

#include <stdarg.h>
#include <terminal.h>

#ifndef _KSTDLOG_H
#define _KSTDLOG_H

#define LOG_TO_SCREEN 0

void klog_set_device(terminal_t* term);
void knotice(const char* fmt, ...);
void kwarn(const char* fmt, ...);
void kerror(const char* fmt, ...);

void panic(const char* fmt, ...);

#endif /* end of include guard: _KSTDLOG_H */
