/****************************************************************************

android_evtloop_stub.cpp -- Android (wxQt) build only

The prebuilt static wxWidgets Qt port in the Qt Android kit was configured
with wxUSE_CONSOLE_EVENTLOOP = 0, so src/unix/evtloopunix.cpp -- which defines
wxGUIAppTraits::GetEventLoopSourcesManager() -- was never compiled.  The Qt
app-traits object still references that symbol, though, because the method is
declared whenever wxUSE_EVENTLOOP_SOURCE = 1 (see wx/unix/apptrait.h).  That
leaves an undefined symbol at link time.

Supply the missing out-of-line definition here.  wxhearts never registers
file-descriptor event-loop sources (wxEventLoopSource / AddSourceForFD), so a
null sources manager is sufficient and correct for this build.

****************************************************************************/

#include <wx/apptrait.h>

#if wxUSE_EVENTLOOP_SOURCE

// wxEventLoopSourcesManagerBase is forward-declared in apptrait.h; a full
// definition isn't needed to return a null pointer of that type.
wxEventLoopSourcesManagerBase* wxGUIAppTraits::GetEventLoopSourcesManager()
{
    return NULL;
}

#endif // wxUSE_EVENTLOOP_SOURCE
