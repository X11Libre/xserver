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

#ifndef CIMDEV_H_
#define CIMDEV_H_

#define CIM_RESERVE_MEM 0x01
#define CIM_FREE_MEM    0x02
#define CIMDEV_MINOR_DEV 156

/* Block flags */
#define CIM_F_FREE    0x01      /* This block is marked as free */
#define CIM_F_CMDBUF  0x02      /* GP command buffer flag */
#define CIM_F_PRIVATE 0x04      /* This block is reserved only for its owner */
#define CIM_F_PUBLIC  0x08      /* This block can be used by the world */

typedef struct {
    /* These fields get populated by the client */
    char owner[10];
    char name[15];
    int flags;
    int size;

    /* These fields are populated by the device */
    unsigned long offset;
} cim_mem_req_t;

typedef struct {
    char owner[10];
    unsigned long offset;
} cim_mem_free_t;

#endif
