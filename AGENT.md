# TODO

[ ] scan for not needed includes of assert.h
[ ] compiler.h --> include of pixman.h needed ?
[ ] compiler.h --> scan drivers for which functions still needed
[ ] compositeext.h --> scan drivers (including nvidia) whether exports are still needed
[ ] damagestr.h --> scan drivers whether the macros are still needed
[ ] dgaproc.h --> scan drivers whether still needed
[ ] dixaccess.h --> scan drivers whether still needed
[ ] replace xace by individual callbacks
[ ] extinit.h --> fix includes --> only need Bool
[ ] scan drivers for fd_notify.h
[ ] scan drivers for global.h still needed
[ ] scan drivers for glx_extinit.h still needed
[ ] scan drivers for i2c_def.h still needed
[ ] scan drivers for micoord.h still needed
[ ] scan drivers for which parts of misc.h still needed
[ ] can we move bgNoneRoot into globals.h ?
[ ] scan drivers for openstr.h still needed
[ ] scan we merge property.h into dix.h ?
[ ] can we drop region.h ?
[ ] scan drivers for rgb.h still needed ?
[ ] scan drivers for servermd.h still needed
[ ] scan drivers for shmint.h still needed
[ ] scan drivers for validate.h still needed
[ ] scan for xaarop.h still needed
[ ] scan drivers for xkbrules.h
[ ] scan drivers for xorgVersion.h
