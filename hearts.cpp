#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"
#include "retro68/trace_retro68.h"
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
#ifdef HEARTS_TRACE
    HeartsTraceInit();
#endif
    HTRACE("OnInit");
    wxInitAllImageHandlers();

    /* Set up filesystem handlers for memory and ZIP archive access */
    HTRACE("image handlers ok");
    wxFileSystem::AddHandler(new wxMemoryFSHandler);
    wxFileSystem::AddHandler(new wxArchiveFSHandler);

    /* Register the compiled XRS archive in the memory filesystem */
    wxMemoryFSHandler::AddFile(wxT("hearts.xrs"), xrc_data, sizeof(xrc_data));

    /* Initialize XRC and load resources from the in-memory archive */
    HTRACE("memory fs ok (%lu bytes)", (unsigned long)sizeof(xrc_data));
    wxXmlResource::Get()->InitAllHandlers();
    HTRACE("XRC handlers ok");
    wxXmlResource::Get()->Load(wxT("memory:hearts.xrs"));

    HTRACE("XRC loaded");
    // Set up config
    wxConfigBase::Set(new wxConfig(wxT("MSHearts")));

    HTRACE("config ok");
    CMainWindow *frame = new CMainWindow();
    HTRACE("main window constructed");
    frame->Show(true);

    HTRACE("main window shown");
#ifdef HEARTS_TRACE
    HeartsTraceAlive(frame);
#endif
    // Post welcome dialog
    wxCommandEvent evt(wxEVT_MENU, IDM_WELCOME);
    frame->GetEventHandler()->QueueEvent(evt.Clone());

    return true;
}
