/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/* debug.h -- wxWidgets port */

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <wx/log.h>

extern wxChar suitid[];
extern wxChar cardid[];

#if defined(_DEBUG) || defined(__WXDEBUG__)

#define TRACE0(x)        wxLogDebug(wxT("%s"), wxT(x))
#define TRACE1(f, a)     wxLogDebug(wxT(f), a)
#define TRACE2(f, a, b)  wxLogDebug(wxT(f), a, b)

#define PLAY(s)  { int v = cd[s].Value2() + 1;\
                   if (v < 11) { wxLogDebug(wxT("play %d"), v); } else\
                   { wxLogDebug(wxT("play %c"), cardid[v-11]); } \
                   wxLogDebug(wxT("%c. "), suitid[cd[s].Suit()]); }

#define CDNAME(c) { int v = c->Value2() + 1;\
                    if (v < 11) { wxLogDebug(wxT("%d"), v); } else\
                    { wxLogDebug(wxT("%c"), cardid[v-11]); } \
                    wxLogDebug(wxT("%c "), suitid[c->Suit()]); }

#define DUMP()

#else

#define TRACE0(x)
#define TRACE1(f, a)
#define TRACE2(f, a, b)
#define PLAY(s)
#define CDNAME(c)
#define DUMP()

#endif

#endif /* __DEBUG_H__ */
