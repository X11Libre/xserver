/* SPDX-License-Identifier: MIT OR X11
 *
 * Copyright © 2024 Enrico Weigelt, metux IT consult <info@metux.net>
 * Copyright © 1999 Egbert Eich
 *
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 */
#ifndef _XSERVER_XF86INT10_H
#define _XSERVER_XF86INT10_H

#include <X11/Xmd.h>
#include <X11/Xdefs.h>
#include "xf86Pci.h"
#include "xf86int10.h"

#ifdef _INT10_PRIVATE

/* int.c */
int int_handler(xf86Int10InfoPtr pInt);

/* helper_exec.c */
int setup_int(xf86Int10InfoPtr pInt);
void finish_int(xf86Int10InfoPtr, int sig);
uint32_t getIntVect(xf86Int10InfoPtr pInt, int num);
void pushw(xf86Int10InfoPtr pInt, uint16_t val);
int run_bios_int(int num, xf86Int10InfoPtr pInt);
void dump_code(xf86Int10InfoPtr pInt);
void dump_registers(xf86Int10InfoPtr pInt);
void stack_trace(xf86Int10InfoPtr pInt);
uint8_t bios_checksum(const uint8_t *start, int size);
void LockLegacyVGA(xf86Int10InfoPtr pInt, legacyVGAPtr vga);
void UnlockLegacyVGA(xf86Int10InfoPtr pInt, legacyVGAPtr vga);

int port_rep_inb(xf86Int10InfoPtr pInt,
                 uint16_t port, uint32_t base, int d_f, uint32_t count);
int port_rep_inw(xf86Int10InfoPtr pInt,
                 uint16_t port, uint32_t base, int d_f, uint32_t count);
int port_rep_inl(xf86Int10InfoPtr pInt,
                 uint16_t port, uint32_t base, int d_f, uint32_t count);
int port_rep_outb(xf86Int10InfoPtr pInt,
                  uint16_t port, uint32_t base, int d_f, uint32_t count);
int port_rep_outw(xf86Int10InfoPtr pInt,
                  uint16_t port, uint32_t base, int d_f, uint32_t count);
int port_rep_outl(xf86Int10InfoPtr pInt,
                  uint16_t port, uint32_t base, int d_f, uint32_t count);

uint8_t x_inb(uint16_t port);
uint16_t x_inw(uint16_t port);
void x_outb(uint16_t port, uint8_t val);
void x_outw(uint16_t port, uint16_t val);
uint32_t x_inl(uint16_t port);
void x_outl(uint16_t port, uint32_t val);

uint8_t Mem_rb(uint32_t addr);
uint16_t Mem_rw(uint32_t addr);
uint32_t Mem_rl(uint32_t addr);
void Mem_wb(uint32_t addr, uint8_t val);
void Mem_ww(uint32_t addr, uint16_t val);
void Mem_wl(uint32_t addr, uint32_t val);

/* helper_mem.c */
void setup_int_vect(xf86Int10InfoPtr pInt);
int setup_system_bios(void *base_addr);
void reset_int_vect(xf86Int10InfoPtr pInt);
void set_return_trap(xf86Int10InfoPtr pInt);
Bool int10skip(const void *options);
Bool int10_check_bios(int scrnIndex, int codeSeg,
                      const unsigned char *vbiosMem);
Bool initPrimary(const void *options);
#ifdef DEBUG
void dprint(unsigned long start, unsigned long size);
#endif

#endif                          /* _INT10_PRIVATE */

#endif /* _XSERVER_XF86INT10_H */
