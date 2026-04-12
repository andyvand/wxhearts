#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/filesys.h>
#include <wx/fs_mem.h>
#include <wx/fs_arc.h>

/* Compiled XRC archive embedded as C array by bin2c */
#include "xrc_data.h"

wxIMPLEMENT_APP(HeartsApp);

wxChar suitid[] = wxT("CDHS");
wxChar cardid[] = wxT("JQKA");

bool HeartsApp::OnInit()
{
    wxInitAllImageHandlers();

    /* Set up filesystem handlers for memory and ZIP archive access */
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxFileSystem::AddHandler(new wxArchiveFSHandler);

    /* Register the compiled XRS archive in the memory filesystem */
    wxMemoryFSHandler::AddFile(wxT("hearts.xrs"), xrc_data, sizeof(xrc_data));

    /* Initialize XRC and load resources from the in-memory archive */
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->Load(wxT("memory:hearts.xrs"));

    // Set up config
    wxConfigBase::Set(new wxConfig(wxT("MSHearts")));

    CMainWindow *frame = new CMainWindow();
    frame->Show(true);

    // Post welcome dialog
    wxCommandEvent evt(wxEVT_MENU, IDM_WELCOME);
    frame->GetEventHandler()->QueueEvent(evt.Clone());

    return true;
}
