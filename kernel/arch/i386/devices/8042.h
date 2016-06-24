/*  Driver for the Intel 8042 PS/2 controller

    Copyright (C) 2016 Arthur M
*/

#include "ioport.h"
#include "../irq.h"

#include <stdint.h>
#include <kstdlog.h>

#ifndef _8042_H
#define _8042_H

#define I8042_CMD 0x64
#define I8042_STATUS 0x64
#define I8042_DATA 0x60

/* Create interrupt handlers and start the controller */
int i8042_init();

/* Send a byte through the controller */
void i8042_send(uint8_t byte, uint8_t port);

/* Receive a byte through the controller */
uint8_t i8042_recv(uint8_t port);

/* Wait until you can send/receive */
void i8042_wait_send();
void i8042_wait_recv();

#endif /* end of include guard: _8042_H */
