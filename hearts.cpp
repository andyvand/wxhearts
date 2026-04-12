#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"
#include <wx/image.h>

wxIMPLEMENT_APP(HeartsApp);

wxChar suitid[] = wxT("CDHS");
wxChar cardid[] = wxT("JQKA");

bool HeartsApp::OnInit()
{
    wxInitAllImageHandlers();

    // Set up config
    wxConfigBase::Set(new wxConfig(wxT("MSHearts")));

    CMainWindow *frame = new CMainWindow();
    frame->Show(true);

    // Post welcome dialog
    wxCommandEvent evt(wxEVT_MENU, IDM_WELCOME);
    frame->GetEventHandler()->QueueEvent(evt.Clone());

    return true;
}
