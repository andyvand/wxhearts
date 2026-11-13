/* trace_retro68.h -- startup tracing for testing the classic Mac OS build
   in an emulator. Only compiled in with -DHEARTS_TRACE=ON (CMake); see
   retro68/trace_retro68.cpp. */

#ifndef TRACE_RETRO68_H
#define TRACE_RETRO68_H

#ifdef HEARTS_TRACE
void HeartsTrace(const char *fmt, ...);
void HeartsTraceInit();
void HeartsTraceAlive(class wxFrame *frame);
#  define HTRACE(...) HeartsTrace(__VA_ARGS__)
#else
#  define HTRACE(...) ((void)0)
#endif

#endif
