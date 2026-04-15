/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

main.cpp -- wxWidgets port

Aug 92, JimH
May 93, JimH    chico port

Main window callback functions
Other CMainWindow member functions are in main2.cpp, welcome.cpp

****************************************************************************/

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include <wx/aboutdlg.h>
#include <wx/iconbndl.h>
#include <wx/xrc/xmlres.h>
#include "debug.h"

#include <cstdlib>
#include <ctime>


// declare static members

wxBrush  CMainWindow::m_BgndBrush;
wxRect   CMainWindow::m_TableRect;

// declare globals

CMainWindow *pMainWnd = nullptr;

MOVE    move;               // describes move for current transaction
MOVE    moveq[8];           // queue of moves waiting to be handled
int     cQdMoves = 0;       // number of moves in above queue
PASS3   passq[4];           // queue of passes waiting to be handled
int     cQdPasses = 0;      // number of passes in above queue
int     nStatusHeight = 0;  // height of status window

// Do not translate these registry/config strings

const wxString szRegPath       = wxT("Hearts");
const wxString regvalSound     = wxT("sound");
const wxString regvalName      = wxT("name");
const wxString regvalRole      = wxT("gamemeister");
const wxString regvalSpeed     = wxT("speed");
const wxString regvalServer    = wxT("server");
const wxString regvalPName[3]  = { wxT("p1name"), wxT("p2name"), wxT("p3name") };

static const wxString regvalCheat = wxT("cheat");


/****************************************************************************

Event table

****************************************************************************/

wxBEGIN_EVENT_TABLE(CMainWindow, wxFrame)
    EVT_MENU(IDM_ABOUT,       CMainWindow::OnAbout)
    EVT_MENU(IDM_NEWGAME,     CMainWindow::OnNewGame)
    EVT_MENU(IDM_OPTIONS,     CMainWindow::OnOptions)
    EVT_MENU(IDM_SOUND,       CMainWindow::OnSound)
    EVT_MENU(IDM_SCORE,       CMainWindow::OnScore)
    EVT_MENU(IDM_CHEAT,       CMainWindow::OnCheat)
    EVT_MENU(IDM_QUOTE,       CMainWindow::OnQuote)
    EVT_MENU(IDM_REF,         CMainWindow::OnRef)
    EVT_MENU(IDM_WELCOME,     CMainWindow::OnWelcome)
    EVT_MENU(IDM_EXIT,        CMainWindow::OnExit)
    EVT_MENU(IDM_BOSSKEY,     CMainWindow::OnBossKey)
    EVT_MENU(IDM_SHOWBUTTON,  CMainWindow::OnShowButton)
    EVT_MENU(IDM_HIDEBUTTON,  CMainWindow::OnHideButton)
    EVT_BUTTON(IDM_BUTTON,    CMainWindow::OnPass)
    EVT_PAINT(CMainWindow::OnPaint)
    EVT_LEFT_DOWN(CMainWindow::OnLeftDown)
    EVT_CLOSE(CMainWindow::OnClose)
    EVT_ERASE_BACKGROUND(CMainWindow::OnEraseBkgnd)
    EVT_CHAR(CMainWindow::OnChar)
    EVT_TIMER(1001,           CMainWindow::OnBadMoveTimer)
    EVT_TIMER(1002,           CMainWindow::OnDispatchTimer)
wxEND_EVENT_TABLE()


/****************************************************************************

CMainWindow constructor

creates green background brush, and main hearts window

****************************************************************************/

CMainWindow::CMainWindow()
    // Use wxDefaultSize here; SetClientSize(WINWIDTH, WINHEIGHT) below
    // fixes the client area to 540x600 *after* the menubar and status
    // bar are created.  Passing an outer size to the wxFrame ctor
    // makes the usable area platform-dependent: on Linux/GTK the menu
    // bar and status bar live inside the outer frame, eating ~60px of
    // vertical space, which pushed the bottom row of cards down and
    // threw off the table proportions.
    : wxFrame(nullptr, wxID_ANY, GetStringResource(IDS_APPNAME),
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX),
      passdir(LEFT_DIR), bAnimating(false), bCheating(false), bSoundOn(false),
      bTimerOn(false), bConstructed(true), m_FatalErrno(0),
      bEnforceFirstBlood(true), m_pButton(nullptr), m_pScoreDlg(nullptr),
      m_badMoveTimer(this, 1001), m_dispatchTimer(this, 1002),
      m_myid(0), bAutostarted(false), bHasSound(false), bNetDdeActive(false)
{
    ::cQdMoves = 0;                 // no moves in move queue
    ::cQdPasses = 0;                // no passes either

    for (int i = 0; i < MAXPLAYER; i++)
        p[i] = nullptr;

    ResetHandInfo(-1);              // set handinfo struct to default values

    m_bkgndcolor = wxColour(0, 127, 0);
    m_BgndBrush = wxBrush(m_bkgndcolor);

    // On wxGTK3 the windowing system asynchronously erases the
    // background between EVT_ERASE_BACKGROUND and EVT_PAINT, which
    // can leave the window painted green for one event-loop tick
    // before the cards are drawn back on top.  Switching to
    // wxBG_STYLE_PAINT tells the system that *all* painting is done
    // in OnPaint, so the green table and the cards are committed in
    // a single atomic repaint and the user never sees a bare green
    // window.  OnEraseBkgnd is left in place as a fallback for ports
    // that still deliver erase-background events.
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    // Set window icon (title bar / taskbar / dock where applicable).
    // On macOS, the Dock/Finder icon comes from the bundle's .icns file;
    // on Windows and Linux this sets the frame icon from the embedded PNG.
    wxIcon appIcon = wxXmlResource::Get()->LoadIcon(wxT("APP_ICON"));
    if (appIcon.IsOk())
        SetIcon(appIcon);

    // Create menu bar

    wxMenuBar *menuBar = new wxMenuBar;
    wxMenu *gameMenu = new wxMenu;
    gameMenu->Append(IDM_NEWGAME, wxT("&New Game\tF2"));
    gameMenu->AppendSeparator();
    gameMenu->Append(IDM_OPTIONS, wxT("&Options...\tF7"));
    gameMenu->AppendCheckItem(IDM_SOUND, wxT("&Sound\tF8"));
    gameMenu->Append(IDM_SCORE, wxT("S&core...\tF9"));
    gameMenu->AppendCheckItem(IDM_CHEAT, wxT("Ch&eat\tF10"));
    gameMenu->AppendSeparator();
    gameMenu->Append(IDM_EXIT, wxT("E&xit"));
    menuBar->Append(gameMenu, wxT("&Game"));

    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(IDM_QUOTE, wxT("&Quote..."));
    helpMenu->AppendSeparator();
    helpMenu->Append(IDM_ABOUT, wxT("&About Hearts"));
    menuBar->Append(helpMenu, wxT("&Help"));
    SetMenuBar(menuBar);

    // Create status bar

    CreateStatusBar();
    SetStatusText(GetStringResource(IDS_INTRO));

    // Now that the menubar and status bar exist, pin the *client*
    // area to WINWIDTH x WINHEIGHT.  On GTK/Linux the menu bar and
    // status bar are drawn inside the outer frame rectangle, so
    // sizing the frame with wxSize(WINWIDTH, WINHEIGHT) in the ctor
    // left the client area ~60px shorter than on Windows/macOS and
    // the bottom row of cards rendered partially off-screen.  Using
    // SetClientSize here makes the playable area identical across
    // platforms regardless of decoration heights.
    SetClientSize(WINWIDTH, WINHEIGHT);

    // Accelerator table

    wxAcceleratorEntry accel[6];
    accel[0].Set(wxACCEL_NORMAL, WXK_F2,     IDM_NEWGAME);
    accel[1].Set(wxACCEL_NORMAL, WXK_F7,     IDM_OPTIONS);
    accel[2].Set(wxACCEL_NORMAL, WXK_F8,     IDM_SOUND);
    accel[3].Set(wxACCEL_NORMAL, WXK_F9,     IDM_SCORE);
    accel[4].Set(wxACCEL_NORMAL, WXK_F10,    IDM_CHEAT);
    accel[5].Set(wxACCEL_NORMAL, WXK_ESCAPE, IDM_BOSSKEY);
    SetAcceleratorTable(wxAcceleratorTable(6, accel));

    // Set up table rect.  Query the real status bar height instead of
    // hard-coding 20px; GTK status bars are typically taller, and using
    // a too-small value left m_TableRect extending under the status bar,
    // pushing the pass button and bottom row of cards out of view.
    wxSize clientSize = GetClientSize();
    wxStatusBar *sbar = GetStatusBar();
    m_StatusHeight = (sbar ? sbar->GetSize().GetHeight() : 20);
    m_TableRect = wxRect(0, 0, clientSize.GetWidth(),
                         clientSize.GetHeight() - m_StatusHeight);

    // Seed random

    srand((unsigned)time(nullptr));

    // Create player 0 (local human)

    pMainWnd = this;
    p[0] = new local_human(0);
    if (!p[0])
    {
        bConstructed = false;
        return;
    }

    // Construct pass button (hidden initially)

    int cxChar = 8, cyChar = 16;
    int btnWidth  = (60 * cxChar) / 4;
    int btnHeight = (14 * cyChar) / 8;
    int bx = (m_TableRect.GetRight() / 2) - (btnWidth / 2);
    int by = m_TableRect.GetBottom() - card::dyCrd - (2 * POPSPACING) - btnHeight;
    m_pButton = new wxButton(this, IDM_BUTTON, wxT(""),
                             wxPoint(bx, by), wxSize(btnWidth, btnHeight));
    m_pButton->Hide();

    // Check for sound capability

    RegEntry Reg(szRegPath);
    bHasSound = SoundInit();
    if (bHasSound && Reg.GetNumber(regvalSound, 1))
    {
        GetMenuBar()->Check(IDM_SOUND, true);
        bSoundOn = true;
    }

    if (Reg.GetNumber(regvalCheat, 0))
    {
        GetMenuBar()->Check(IDM_CHEAT, true);
        bCheating = true;
    }

    // Animation speed

    card c;
    int  nStepSize;
    long dwSpeed = Reg.GetNumber(regvalSpeed, IDC_NORMAL);

    if (dwSpeed == IDC_FAST)
        nStepSize = 60;
    else if (dwSpeed == IDC_SLOW)
        nStepSize = 5;
    else
        nStepSize = 15;

    c.SetStepSize(nStepSize);
}


/****************************************************************************

CMainWindow::OnAbout

displays about box

****************************************************************************/

void CMainWindow::OnAbout(wxCommandEvent &event)
{
    wxAboutDialogInfo info;
    info.SetName(GetStringResource(IDS_APPNAME));
    info.SetDescription(GetStringResource(IDS_CREDITS));
    info.SetCopyright(wxT("Copyright (c) Microsoft Corp., 1991, 1992"));
    wxAboutBox(info, this);
}


/****************************************************************************

CMainWindow::OnQuote

displays quote box and plays quote.

****************************************************************************/

void CMainWindow::OnQuote(wxCommandEvent &event)
{
    CQuoteDlg quote(this);
    HeartsPlaySound(SND_QUOTE);
    quote.ShowModal();
    HeartsPlaySound(OFF);
}


/****************************************************************************

CMainWindow::OnChar -- looks for space, plays first legal move, or pushes button

****************************************************************************/

void CMainWindow::OnChar(wxKeyEvent &event)
{
    // We know the cast below is legal because position 0 is always
    // the local human.

    local_human *p0 = (local_human *)p[0];

    modetype mode = p0->GetMode();

    if ((event.GetUnicodeKey() != ' ') || (p0->IsTimerOn()))
    {
        event.Skip();
        return;
    }

    if (mode != PLAYING)
        return;

    p0->SetMode(WAITING);

    wxPoint loc;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (p0->GetCardLoc(s, loc))
        {
            if (p0->PlayCard(loc.x, loc.y, handinfo, bCheating, false))
            {
                return;
            }
        }
    }

    p0->SetMode(PLAYING);
}


/****************************************************************************

CMainWindow::OnCheat -- toggles bCheating used to show all cards face up.

****************************************************************************/

void CMainWindow::OnCheat(wxCommandEvent &event)
{
    RegEntry Reg(szRegPath);

    bCheating = !bCheating;
    Refresh();                      // redraw main hearts window

    GetMenuBar()->Check(IDM_CHEAT, bCheating);

    if (bCheating)
        Reg.SetValue(regvalCheat, (long)1);
    else
        Reg.DeleteValue(regvalCheat);
}


/****************************************************************************

CMainWindow::OnClose -- cleans up background brush, deletes players, etc.

****************************************************************************/

void CMainWindow::OnClose(wxCloseEvent &event)
{
    for (int i = 0; i < MAXPLAYER; i++)
    {
        if (p[i])
        {
            delete p[i];
            p[i] = nullptr;
        }
    }

    if (m_pButton)
    {
        m_pButton->Destroy();
        m_pButton = nullptr;
    }

    m_dispatchTimer.Stop();
    m_badMoveTimer.Stop();

    {
        RegEntry Reg(szRegPath);
        Reg.FlushKey();
    }

    Destroy();
}


/****************************************************************************

CMainWindow::OnEraseBkgnd -- required to draw background green

****************************************************************************/

void CMainWindow::OnEraseBkgnd(wxEraseEvent &event)
{
    wxDC *pDC = event.GetDC();
    if (!pDC) return;

    pDC->SetBrush(m_BgndBrush);
    pDC->SetPen(*wxTRANSPARENT_PEN);
    pDC->DrawRectangle(0, 0, WINWIDTH, WINHEIGHT);
}


/****************************************************************************

CMainWindow::OnLeftDown

Handles human selecting card to play or pass.

****************************************************************************/

void CMainWindow::OnLeftDown(wxMouseEvent &event)
{
    // We know the cast below is legal because position 0 is always
    // the local human.

    local_human *p0 = (local_human *)p[0];

    if (p0->IsTimerOn())            // ignore mouse clicks if timer running
        return;

    modetype mode = p0->GetMode();

    if (mode == SELECTING)
    {
        p0->PopCard(m_BgndBrush, event.GetX(), event.GetY());
        return;
    }
    else if (mode != PLAYING)
        return;

    p0->SetMode(WAITING);
    if (p0->PlayCard(event.GetX(), event.GetY(), handinfo, bCheating))
        return;

    // move wasn't legal, so back to PLAYING mode

    p0->SetMode(PLAYING);
}


/****************************************************************************

CMainWindow::OnNewGame

****************************************************************************/

void CMainWindow::OnNewGame(wxCommandEvent &event)
{
    passdir = LEFT_DIR;             // each new game must start with LEFT_DIR

    bAutostarted = false;           // means dealer has agreed to play at least

    if (role == GAMEMEISTER)
    {
        for (int i = 1; i < MAXPLAYER; i++)
        {
            if (!p[i])
            {
                p[i] = new computer(i);
                if (!p[i])
                {
                    bConstructed = false;
                    return;
                }
            }
        }

        m_gamenumber = rand();
    }

    ResetHandInfo(-1);

    srand(m_gamenumber);

    {
        CScoreDlg score(this);
        score.ResetScore();
    }                               // destruct score

    Shuffle();
}


/****************************************************************************

CMainWindow::OnOptions -- user requests options dialog from menu

****************************************************************************/

void CMainWindow::OnOptions(wxCommandEvent &event)
{
    COptionsDlg optionsdlg(this);
    optionsdlg.ShowModal();
}


/****************************************************************************

CMainWindow::OnPaint

****************************************************************************/

void CMainWindow::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);

    // During card animation, do NOT paint anything -- not even the
    // background.  GlideStep draws each frame directly through a
    // wxClientDC and then calls Refresh(false) + Update() to flush it
    // to the screen.  With wxBG_STYLE_PAINT we own the background, so
    // if OnPaint painted the green table here it would immediately
    // overwrite the animation frame the client DC just drew, and the
    // early-return below would leave the window a solid green with no
    // cards on it.  That is exactly the "click a card to play and the
    // screen stays green" symptom seen on Linux Mint / wxGTK3.
    //
    // Returning without touching the DC (other than creating the
    // PaintDC to consume the event) leaves the pixels the client DC
    // rendered intact, which is what the MFC/Win32 version relied on
    // via Refresh(false) suppressing the erase-background.
    if (bAnimating)
        return;

    // With wxBG_STYLE_PAINT the system no longer erases the background
    // for us, so OnPaint must paint the green table itself.  Doing it
    // here (rather than in OnEraseBkgnd) keeps background + cards in a
    // single atomic repaint, which avoids the "all-green, no cards"
    // flicker seen on Linux Mint / wxGTK3 after Refresh().
    {
        wxSize sz = GetClientSize();
        dc.SetBrush(m_BgndBrush);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, sz.GetWidth(), sz.GetHeight());
    }

    // players must be painted in order starting with playerled so that
    // cards in centre overlap correctly

    if (bConstructed)
    {
        int start = Id2Pos(handinfo.playerled % 4);

        // check that someone has started

        if (start >= 0)
        {
            for (int i = start; i < (MAXPLAYER + start); i++)
            {
                int pos = i % 4;
                if (p[pos])
                {
                    if (p[pos]->GetMode() == SCORING)
                    {
                        p[pos]->DisplayHeartsWon(dc);
                    }
                    else
                    {
                        p[pos]->Draw(dc, bCheating);
                        p[pos]->MarkSelectedCards(dc);
                    }
                }
            }
        }
    }
}


/****************************************************************************

CMainWindow::OnPass

This function handles the local human pressing the button either to
pass selected cards or to accept cards passed.

****************************************************************************/

void CMainWindow::OnPass(wxCommandEvent &event)
{
    if (p[0]->GetMode() == ACCEPTING)       // OK (accepting passed cards)
    {
        m_pButton->Hide();
        m_pButton->SetLabel(wxT(""));
        p[0]->SetMode(WAITING);             // local human pushed the button

        wxRect rect;
        p[0]->GetCoverRect(rect);

        for (SLOT s = 0; s < MAXSLOT; s++)
            p[0]->Select(s, false);

        RefreshRect(rect, true);
        Update();

        FirstMove();

        for (int i = 0; i < ::cQdMoves; i++)
            HandleMove(::moveq[i]);

        ::cQdMoves = 0;
        ::cQdPasses = 0;

        return;
    }

    m_pButton->Enable(false);
    p[0]->SetMode(DONE_SELECTING);

    bool bReady = true;

    for (int i = 1; i < MAXPLAYER; i++)
        if (p[i]->GetMode() != DONE_SELECTING)
            bReady = false;

    if (!bReady)
        p[0]->UpdateStatus(IDS_PASSWAIT);

    // In local mode (gamemeister), pass data is handled directly.
    // No DDE needed.

    if (bReady)
        HandlePassing();
}


/****************************************************************************

CMainWindow::OnRef

After a human or a computer plays a card, they must
PostMessage(WM_COMMAND, IDM_REF) -- in wxWidgets: QueueEvent(IDM_REF)
which causes this routine (the referee) to be called.

Ref does the following:
    - updates handinfo data struct
    - calls HeartsPlaySound() if appropriate
    - determines if the hand is over or, if not, whose turn is next

****************************************************************************/

void CMainWindow::OnRef(wxCommandEvent &event)
{
    OnRef();
}

void CMainWindow::OnRef()
{
    card *c = handinfo.cardplayed[handinfo.turn];

    if (!handinfo.bHeartsBroken)
    {
        if (c->Suit() == HEARTS)
        {
            handinfo.bHeartsBroken = true;
            HeartsPlaySound(SND_BREAK);
        }
    }

    if (c->ID() == BLACKLADY)
    {
        handinfo.bQSPlayed = true;
        HeartsPlaySound(SND_QUEEN);
    }

    int pos = Id2Pos(handinfo.turn);
    SLOT slot = p[pos]->GetSlot(handinfo.cardplayed[handinfo.turn]->ID());

    p[pos]->GlideToCentre(slot, pos == 0 ? true : bCheating);

    handinfo.turn++;
    handinfo.turn %= 4;

    int newpos = Id2Pos(handinfo.turn);

    if (handinfo.turn == handinfo.playerled)
    {
        EndHand();
    }
    else
    {
        p[newpos]->SelectCardToPlay(handinfo, bCheating);

        if (newpos != 0)
            ((local_human *)p[0])->WaitMessage(p[newpos]->GetName());
    }
}


/****************************************************************************

CMainWindow::OnScore -- user requests score dialog from menu

****************************************************************************/

extern int score[MAXPLAYER];
extern int nHandsPlayed;

void CMainWindow::OnScore(wxCommandEvent &event)
{
    CScoreDlg scoredlg(this);
    scoredlg.ShowModal();
}


/****************************************************************************

CMainWindow::DoSort

****************************************************************************/

void CMainWindow::DoSort()
{
    for (int i = 0; i < (bCheating ? MAXPLAYER : 1); i++)
    {
        wxRect rect;
        int id;             // card in play for this player

        if (handinfo.cardplayed[i] == nullptr)
            id = EMPTY;
        else
            id = handinfo.cardplayed[i]->ID();

        p[i]->Sort();

        if (id != EMPTY)    // if this player has a card in play, restore it
        {
            for (SLOT s = 0; s < MAXSLOT; s++)
            {
                if (p[i]->GetID(s) == id)
                {
                    handinfo.cardplayed[i] = p[i]->Card(s);
                    break;
                }
            }
        }

        p[i]->GetCoverRect(rect);
        RefreshRect(rect, true);
    }
}


/****************************************************************************

CMainWindow::OnSound()

request sound on or off from menu.

****************************************************************************/

void CMainWindow::OnSound(wxCommandEvent &event)
{
    RegEntry Reg(szRegPath);

    bSoundOn = !bSoundOn;

    GetMenuBar()->Check(IDM_SOUND, bSoundOn);

    if (bSoundOn)
        Reg.SetValue(regvalSound, (long)1);
    else
        Reg.DeleteValue(regvalSound);
}


/****************************************************************************

CMainWindow::OnBossKey -- minimize the window

****************************************************************************/

void CMainWindow::OnBossKey(wxCommandEvent &event)
{
    Iconize(true);
}


/****************************************************************************

CMainWindow::OnExit

****************************************************************************/

void CMainWindow::OnExit(wxCommandEvent &event)
{
    bConstructed = false;
    Close();
}


/****************************************************************************

CMainWindow::OnShowButton / OnHideButton

****************************************************************************/

void CMainWindow::OnShowButton(wxCommandEvent &event)
{
    if (m_pButton)
    {
        m_pButton->Enable(true);
        m_pButton->SetFocus();
    }
}

void CMainWindow::OnHideButton(wxCommandEvent &event)
{
    if (m_pButton)
        m_pButton->Enable(false);
}


/****************************************************************************

CMainWindow::OnBadMoveTimer

Called when the bad move highlight timer fires. Kills timer and
refreshes the highlighted card rect.

****************************************************************************/

void CMainWindow::OnBadMoveTimer(wxTimerEvent &event)
{
    m_badMoveTimer.Stop();
    local_human::ResetTimer();
    RefreshRect(m_badMoveRect, false);
}


/****************************************************************************

CMainWindow::StartBadMoveTimer

Called by local_human::StartTimer to flash an invalid card.
Stores the card rect and starts the bad move timer.

****************************************************************************/

void CMainWindow::StartBadMoveTimer(const wxRect &rect)
{
    m_badMoveRect = rect;
    if (!m_badMoveTimer.StartOnce(250))
    {
        // If timer fails, just refresh immediately
        local_human::ResetTimer();
        RefreshRect(m_badMoveRect, false);
    }
}


/****************************************************************************

CMainWindow::OnDispatchTimer

****************************************************************************/

void CMainWindow::OnDispatchTimer(wxTimerEvent &event)
{
    DispatchCards();
}
