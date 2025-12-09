#ifndef TESTS_H
#define TESTS_H

extern int verbose;
#define dbg(...) if (verbose) printf("DBG" __VA_ARGS__);

#define DECLARE_WRAP_FUNCTION(f_, rval_, ...) \
    extern rval_ (*wrapped_ ## f_)(__VA_ARGS__) \

#define IMPLEMENT_WRAP_FUNCTION(f_, ...) \
    if (wrapped_ ## f_) wrapped_ ## f_(__VA_ARGS__); \
    else __real_ ## f_(__VA_ARGS__)

#define IMPLEMENT_WRAP_FUNCTION_WITH_RETURN(f_, ...) \
    if (wrapped_ ## f_) return wrapped_ ## f_(__VA_ARGS__); \
    else return __real_ ## f_(__VA_ARGS__)

#define WRAP_FUNCTION(f_, rval_, ...) \
    rval_ (*wrapped_ ## f_)(__VA_ARGS__); \
    extern rval_ __real_ ## f_(__VA_ARGS__); \
    rval_ __wrap_ ## f_(__VA_ARGS__); \
    rval_ __wrap_ ## f_(__VA_ARGS__)

typedef void (*testfunc_t)(void);

const testfunc_t* fixes_test(void);
const testfunc_t* hashtabletest_test(void);
const testfunc_t* input_test(void);
const testfunc_t* list_test(void);
const testfunc_t* misc_test(void);
const testfunc_t* sha1_test(void);
const testfunc_t* signal_logging_test(void);
const testfunc_t* string_test(void);
const testfunc_t* touch_test(void);
const testfunc_t* xfree86_test(void);
const testfunc_t* xkb_test(void);
const testfunc_t* xtest_test(void);
const testfunc_t* protocol_xchangedevicecontrol_test(void);
const testfunc_t* protocol_xiqueryversion_test(void);
const testfunc_t* protocol_xiquerydevice_test(void);
const testfunc_t* protocol_xiselectevents_test(void);
const testfunc_t* protocol_xigetselectedevents_test(void);
const testfunc_t* protocol_xisetclientpointer_test(void);
const testfunc_t* protocol_xigetclientpointer_test(void);
const testfunc_t* protocol_xipassivegrabdevice_test(void);
const testfunc_t* protocol_xiquerypointer_test(void);
const testfunc_t* protocol_xiwarppointer_test(void);
const testfunc_t* protocol_eventconvert_test(void);
const testfunc_t* xi2_test(void);

#endif /* TESTS_H */

