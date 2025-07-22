/*
 * Copyright (c) 2006 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Neither the name of the Advanced Micro Devices, Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 */

 /*
  * Base include file for the Cimarron library. This file should be modified
  * and included in any Cimarron-based project.
  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>

/*----------------------------------------------------------------------*/
/* MODULE SETTINGS                                                      */
/* The following #defines affect which modules are included in the      */
/* project.                                                             */
/*----------------------------------------------------------------------*/

#define CIMARRON_INCLUDE_GP                1
#define CIMARRON_INCLUDE_VG                1
#define CIMARRON_INCLUDE_VIP               1
#define CIMARRON_INCLUDE_VOP               1
#define CIMARRON_INCLUDE_VIDEO             1
#define CIMARRON_INCLUDE_INIT              1

#define CIMARRON_INCLUDE_VG_READ_ROUTINES  1
#define CIMARRON_INCLUDE_DF_READ_ROUTINES  1
#define CIMARRON_INCLUDE_VIP_READ_ROUTINES 1
#define CIMARRON_INCLUDE_VOP_READ_ROUTINES 1

/*----------------------------------------------------------------------*/
/* HARDWARE ACCESS SETTINGS                                             */
/* The following #defines affect how the Cimarron macros access the     */
/* hardware.  The hardware access macros are broken up into groups.     */
/* Each group includes an enabling #define as well as several #define   */
/* options that modify the macro configuration that is included.        */
/* If the enabling define is deleted or all options are set to 0, the   */
/* corresponding macros must be implemented by the user.   The          */
/* combinations are explained as follows:                               */
/* must be manually defined by the user. This allows a user to use the  */
/* cim_defs.h file for only those macros that suit the needs of his/her */
/* project.  For example, a user may need custom implementations of the */
/* I/O and MSR macros, but may still want to use the default macros to  */
/* read and write hardware registers. The combinations are explained as */
/* follows:                                                             */
/*                                                                      */
/* Register Group:                                                      */
/*   Disabling define:                                                  */
/*       CIMARRON_EXCLUDE_REGISTER_ACCESS_MACROS                        */
/*          Define this setting to exclude the register access macros.  */
/*          This setting is the inverse of the other group settings in  */
/*          that these macros are included by default.  This allows the */
/*          cim_defs.h file to be included outside of cimarron.c for    */
/*          basic operations.                                           */
/*                                                                      */
/* Memory Group:                                                        */
/*   Enabling define:                                                   */
/*       CIMARRON_INCLUDE_STRING_MACROS                                 */
/*   Options:                                                           */
/*       CIMARRON_OPTIMIZE_ASSEMBLY                                     */
/*           Set to 1 to allow the use of inline assembly when writing  */
/*           large chunks of data to memory.  Essentially, this allows  */
/*           a rep movsd in place of a slower C for-loop.               */
/*      CIMARRON_OPTIMIZE_FORLOOP                                       */
/*           Define for C only data writes.                             */
/*                                                                      */
/* MSR Group:                                                           */
/*   Enabling define:                                                   */
/*      CIMARRON_INCLUDE_MSR_MACROS                                     */
/*   Options:                                                           */
/*      CIMARRON_MSR_DIRECT_ASM                                         */
/*          Set to 1 to allow the use of the rdmsr and wrmsr opcodes in */
/*          inline assembly.                                            */
/*      CIMARRON_MSR_VSA_IO                                             */
/*          Set to 1 to access MSRs using a VSAII virtual register.     */
/*      CIMARRON_MSR_KERNEL_ROUTINE                                     */
/*          Set to 1 to access MSRs using a wrapper routine in the      */
/*          Linux kernel.                                               */
/*                                                                      */
/* IO Group:                                                            */
/*   Enabling define:                                                   */
/*      CIMARRON_INCLUDE_IO_MACROS                                      */
/*   Options:                                                           */
/*      CIMARRON_IO_DIRECT_ACCESS                                       */
/*          Set to 1 to perform IO accesses using inline assembly.      */
/*      CIMARRON_IO_ABSTRACTED_ASM                                      */
/*          Set to 1 to perform IO using abstracted IO in Linux.        */
/*                                                                      */
/* Custom Group:                                                        */
/*    Disabling define:                                                 */
/*      CIMARRON_EXCLUDE_CUSTOM_MACROS                                  */
/*          By default, the custom macros (the macros used by           */
/*          gp_custom_convert_blt) are mapped to the normal command     */
/*          string macros.  Setting this to 1 allows the user to        */
/*          create a custom implementation.                             */
/*----------------------------------------------------------------------*/

/* UNCOMMENT THE FOLLOWING LINE TO EXCLUDE BASIC REGISTER ACCESS MACROS */

/* #define CIMARRON_EXCLUDE_REGISTER_ACCESS_MACROS */

#define CIMARRON_INCLUDE_STRING_MACROS
#define CIMARRON_OPTIMIZE_ASSEMBLY         0
#define CIMARRON_OPTIMIZE_FORLOOP          0
#define CIMARRON_OPTIMIZE_ABSTRACTED_ASM   1

#define CIMARRON_INCLUDE_MSR_MACROS
#define CIMARRON_MSR_DIRECT_ASM            0
#define CIMARRON_MSR_VSA_IO                0
#define CIMARRON_MSR_ABSTRACTED_ASM        0
#define CIMARRON_MSR_KERNEL_ROUTINE        0
#define CIMARRON_MSR_HOOKS                 1

#define CIMARRON_INCLUDE_IO_MACROS
#define CIMARRON_IO_DIRECT_ACCESS          0
#define CIMARRON_IO_ABSTRACTED_ASM         1

/*
 * UNCOMMENT THE FOLLOWING LINE TO IMPLEMENT CUSTOM MACROS FOR
 * GP_CUSTOM_CONVERT_BLT
 */

/* #define CIMARRON_EXCLUDE_CUSTOM_MACROS */

/* ROUTINE DEFINITIONS */
/* All routines have a prototype, even those that are not included    */
/* via #ifdefs.  This prevents the user from having to include the    */
/* correct #defines anywhere he/she wants to call a Cimarron routine. */

#include "cim_rtns.h"

/*----------------------------------------------------------------------*/
/* MODULE VARIABLES                                                     */
/* The following #defines affect how global variables in each Cimarron  */
/* module are defined.  These variables can be made static (to prevent  */
/* naming conflicts) or they can be defined without the static keyword  */
/* (to allow extern references).                                        */
/*----------------------------------------------------------------------*/

#if 1
#define CIMARRON_STATIC static
#else
#define CIMARRON_STATIC
#endif

/*----------------------------------------------------------------------*/
/* CIMARRON GLOBAL VARIABLES                                            */
/* These globals are used by the hardware access macros.  They must be  */
/* initialized by the application to point to the memory-mapped         */
/* registers of their respective blocks.                                */
/*----------------------------------------------------------------------*/

unsigned char *cim_gp_ptr = (unsigned char *) 0;
unsigned char *cim_fb_ptr = (unsigned char *) 0;
unsigned char *cim_cmd_base_ptr = (unsigned char *) 0;
unsigned char *cim_cmd_ptr = (unsigned char *) 0;
unsigned char *cim_vid_ptr = (unsigned char *) 0;
unsigned char *cim_vip_ptr = (unsigned char *) 0;
unsigned char *cim_vg_ptr = (unsigned char *) 0;

/* Define hooks for reading and writing MSRs - this is a major hack
 * to share the MSR code with the GX code */

#ifdef CIMARRON_MSR_HOOKS
void (*cim_rdmsr)(uint32_t, uint32_t *, uint32_t *);
void (*cim_wrmsr)(uint32_t, uint32_t, uint32_t);
#endif

/*----------------------------------------------------------------------*/
/* INCLUDE RELEVANT CIMARRON HEADERS                                    */
/*----------------------------------------------------------------------*/

/* HARDWARE REGISTER DEFINITIONS */

#include "cim_regs.h"

/* HARDWARE ACCESS MACROS */

#include "cim_defs.h"

/*----------------------------------------------------------------------*/
/* CIMARRON MODULES                                                     */
/* Modules and sub-modules are included based on user settings.  Note   */
/* that excluding one or more modules may result in functionality       */
/* holes.                                                               */
/*----------------------------------------------------------------------*/

/* GRAPHICS PROCESSOR */

#if CIMARRON_INCLUDE_GP
#include "cim_gp.c"
#endif

/* VIDEO GENERATOR */

#if CIMARRON_INCLUDE_VG
#include "cim_modes.c"
#include "cim_vg.c"
#endif

/* DISPLAY FILTER */

#if CIMARRON_INCLUDE_VIDEO
#include "cim_filter.c"
#include "cim_df.c"
#endif

/* INITIALIZATION AND DETECTION */

#if CIMARRON_INCLUDE_INIT
#include "cim_init.c"
#endif

/* VIP SUPPORT */

#if CIMARRON_INCLUDE_VIP
#include "cim_vip.c"
#endif

/* VOP SUPPORT */

#if CIMARRON_INCLUDE_VOP
#include "cim_vop.c"
#endif

/* MSR ACCESS */
/* This module is used to access machine-specific registers. */
/* It cannot be excluded from a project.                     */

#include "cim_msr.c"
