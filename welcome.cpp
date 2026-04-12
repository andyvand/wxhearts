/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

welcome.cpp -- wxWidgets port

Aug 92, JimH
May 93, JimH    chico port

more CMainWindow member functions

CTheApp::InitInstance() posts a IDM_WELCOME message as soon as it has
constructed and shown the main window.  This file includes that message's
handler (OnWelcome) and some support routines.

****************************************************************************/

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"


/****************************************************************************

CMainWindow::OnWelcome()

Pop up the Welcome dialog.  In the wxWidgets port there is no network
support, so we always become gamemeister and start the game immediately.

****************************************************************************/

void CMainWindow::OnWelcome(wxCommandEvent &event)
{
    CWelcomeDlg welcome(this);

    if (!bAutostarted)
    {
        if (welcome.ShowModal() == wxID_CANCEL)
        {
            Close();
            return;
        }
    }

    /* Always gamemeister in local mode */
    role = GAMEMEISTER;
    m_myid = 0;

    wxClientDC dc(this);
    wxString name = welcome.GetMyName();
    if (name.IsEmpty())
        name = GetStringResource(IDS_DEALER);

    p[0]->SetName(name, dc);
    p[0]->DisplayName(dc);
    p[0]->SetStatus(IDS_GMWAIT);

    /* Start game immediately (no network waiting) */
    wxCommandEvent newGameEvt(wxEVT_MENU, IDM_NEWGAME);
    GetEventHandler()->QueueEvent(newGameEvt.Clone());
}


/****************************************************************************

CMainWindow::FatalError()

A static bool prevents this function from being called reentrantly.  One is
enough, and more than one leaves things in bad states.  The parameter is
the IDS_X constant that identifies the string to display.

There is also a check that we don't try to shut down while the score dialog
is displayed.  This avoids some nasty traps when the score dialog
doesn't shut down properly.

****************************************************************************/

void CMainWindow::FatalError(int errorno)
{
    if (p[0] && p[0]->GetMode() == SCORING)
    {
        m_FatalErrno = errorno;
        return;
    }

    static bool bClosing = false;
    if (bClosing) return;
    bClosing = true;

    if (errorno != -1)
    {
        wxString msg = GetStringResource(errorno);
        wxString caption = GetStringResource(IDS_APPNAME);
        wxMessageBox(msg, caption, wxICON_ERROR);
    }

    Close(true);
}


/****************************************************************************

CMainWindow::GameOver

****************************************************************************/

void CMainWindow::GameOver()
{
    wxClientDC dc(this);
    Refresh();
    p[0]->SetMode(STARTING);
    p[0]->SetScore(0);

    for (int i = 1; i < MAXPLAYER; i++)
    {
        delete p[i];
        p[i] = nullptr;
    }

    /* Start new game */
    p[0]->SetStatus(IDS_GMWAIT);
    p[0]->DisplayName(dc);

    wxCommandEvent evt(wxEVT_MENU, IDM_NEWGAME);
    GetEventHandler()->QueueEvent(evt.Clone());
}


/****************************************************************************

CMainWindow::PlayerQuit

In local mode, just replace with a computer player.

****************************************************************************/

void CMainWindow::PlayerQuit(int id)
{
    int pos = Id2Pos(id);
    if (p[pos])
    {
        wxString name = p[pos]->GetName();
        int sc = p[pos]->GetScore();
        delete p[pos];
        p[pos] = new computer(pos);
        wxClientDC dc(this);
        p[pos]->SetName(name, dc);
        p[pos]->SetScore(sc);
    }
}
