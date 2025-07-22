/*
 * Copyright (c) 2007 Advanced Micro Devices, Inc.
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

#ifndef AMD_BLEND_H_
#define AMD_BLEND_H_

#define GEODEPTR_FROM_PIXMAP(x)		\
		GEODEPTR(xf86Screens[(x)->drawable.pScreen->myNum])
#define GEODEPTR_FROM_SCREEN(x)		\
		GEODEPTR(xf86Screens[(x)->myNum])
#define GEODEPTR_FROM_PICTURE(x)	\
		GEODEPTR(xf86Screens[(x)->pDrawable->pScreen->myNum])

#define usesPasses(op) (((		\
   ( 1 << PictOpAtop ) | 		\
   ( 1 << PictOpAtopReverse ) | \
   ( 1 << PictOpXor ) | 		\
   0 ) >> (op)) & 1)

/* pass1 or pass2 */
#define usesSrcAlpha(op) ((( 		\
   ( 1 << PictOpOver ) | 			\
   ( 1 << PictOpInReverse  ) | 		\
   ( 1 << PictOpOutReverse  ) | 	\
   ( 1 << PictOpAtop ) | 			\
   ( 1 << PictOpAtopReverse ) | 	\
   ( 1 << PictOpXor ) | 			\
   0 ) >> (op)) & 1)

/* pass1 or pass2 */
#define usesDstAlpha(op) ((( 		\
   ( 1 << PictOpOverReverse ) | 	\
   ( 1 << PictOpIn ) | 				\
   ( 1 << PictOpOut ) | 			\
   ( 1 << PictOpAtop ) | 			\
   ( 1 << PictOpAtopReverse ) | 	\
   ( 1 << PictOpXor ) | 			\
   0 ) >> (op)) & 1)

/* non 2 pass ops */
#define usesChanB0(op) ((( 			\
   ( 1 << PictOpOver ) | 			\
   ( 1 << PictOpOverReverse ) | 	\
   ( 1 << PictOpIn ) | 				\
   ( 1 << PictOpInReverse ) | 		\
   ( 1 << PictOpOut ) | 			\
   ( 1 << PictOpOutReverse ) | 		\
   ( 1 << PictOpAdd ) | 			\
   0 ) >> (op)) & 1)

/* pass 1 ops */
#define usesChanB1(op) ((( 			\
   ( 1 << PictOpAtop ) | 			\
   ( 1 << PictOpAtopReverse ) | 	\
   ( 1 << PictOpXor ) | 			\
   0 ) >> (op)) & 1)

/* pass 2 ops */
#define usesChanB2(op) ((( 			\
   ( 1 << PictOpAtop ) | 			\
   ( 1 << PictOpAtopReverse ) | 	\
   ( 1 << PictOpXor ) | 			\
   0 ) >> (op)) & 1)

#endif
