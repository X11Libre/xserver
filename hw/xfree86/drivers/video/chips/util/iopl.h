#ifdef __NetBSD__
#  include <sys/types.h>
#  include <machine/pio.h>
#  include <machine/sysarch.h>
#else
#  if defined(__linux__)
/* Can't because <sys/iopl.h> provides conflicting inb, outb, etc
 * #    include <sys/io.h>
 */
int iopl(int level);
#  endif
#  if defined(SVR4) && defined(i386) && defined(sun)
#    include <sys/types.h>
#    include <sys/sysi86.h>
#    include <sys/v86.h>
#    include <sys/psw.h>
#  endif
#  include "AsmMacros.h"
#endif /* NetBSD */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __NetBSD__
#  define SET_IOPL() i386_iopl(3)
#  define RESET_IOPL() i386_iopl(0)
#else
#  if defined(SVR4) && defined(i386)
#    ifndef SI86IOPL
#      define SET_IOPL() sysi86(SI86V86,V86SC_IOPL,PS_IOPL)
#      define RESET_IOPL() sysi86(SI86V86,V86SC_IOPL,0)
#    else
#      define SET_IOPL() sysi86(SI86IOPL,3)
#      define RESET_IOPL() sysi86(SI86IOPL,0)
#    endif
#  else
#    ifdef linux
#      define SET_IOPL() iopl(3)
#      define RESET_IOPL() iopl(0)
#    else
#      define SET_IOPL() (void)0
#      define RESET_IOPL() (void)0
#    endif
#  endif
#endif
