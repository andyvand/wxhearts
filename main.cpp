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
    EVT_COMMAND(IDM_BUTTON, wxEVT_BUTTON, CMainWindow::OnPass)
    EVT_PAINT(CMainWindow::OnPaint)
    EVT_SIZE(CMainWindow::OnSize)
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
              wxDEFAULT_FRAME_STYLE),
      passdir(LEFT_DIR), bAnimating(false), bCheating(false), bSoundOn(false),
      bTimerOn(false), bConstructed(true), m_FatalErrno(0),
      bEnforceFirstBlood(true),
      m_bPassBtnVisible(false), m_bPassBtnEnabled(false),
      m_bPassBtnPressed(false),
      m_pScoreDlg(nullptr),
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

    // Paint-only background: OnPaint fills the whole client area
    // (either by blitting the scaled backbuffer or by drawing the
    // letterbox bars), so we don't want the system to erase it for
    // us.  On wxGTK3 this also prevents a one-tick flash of green
    // between EVT_ERASE_BACKGROUND and EVT_PAINT.
    //
    // Previously we also called SetBackgroundColour(m_bkgndcolor) so
    // that on macOS live resize the newly exposed regions around the
    // native pass button wouldn't flash black.  Now that the pass
    // button is custom-drawn into the backbuffer (no native child),
    // there's no need to tint the frame's content view -- and doing
    // so would propagate to the status bar and render it green.
    SetBackgroundStyle(wxBG_STYLE_PAINT);

#ifdef __WXMAC__
    // Enable CoreAnimation backing for the frame on macOS.  With this
    // turned on, the window's layer retains its previous contents
    // during live resize, so the pass-selection scene stays on screen
    // while the user drags the resize handle -- our OnPaint then
    // overwrites it with the newly-scaled scene when it fires.
    SetDoubleBuffered(true);
#endif

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

    // Pin the minimum client area to the original playable size so
    // the user can grow the window and maximize it, but never shrink
    // it below the area required to draw the full hand and table.
    // The game layout (cards, pass button, m_TableRect) stays
    // anchored at the top-left at its fixed logical size; extra
    // client area is simply filled with the green table background.
    SetMinClientSize(wxSize(WINWIDTH, WINHEIGHT));

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

    // Allocate the persistent backbuffer and pre-fill it with the
    // green table colour.  OnPaint will refill it with the full scene
    // on first paint; we pre-fill here so the window isn't white for
    // the instant between show and first paint.
    m_backbuffer = wxBitmap(clientSize.GetWidth(), clientSize.GetHeight());
    if (m_backbuffer.IsOk())
    {
        wxMemoryDC mdc;
        mdc.SelectObject(m_backbuffer);
        mdc.SetBrush(m_BgndBrush);
        mdc.SetPen(*wxTRANSPARENT_PEN);
        mdc.DrawRectangle(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
        mdc.SelectObject(wxNullBitmap);
    }

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

    // Compute pass-button geometry in logical coords.  The button is
    // drawn into the backbuffer in RenderScene (not a native wxButton),
    // so there's no child window to move on resize -- scaling comes
    // for free via StretchBlit.

    int cxChar = 8, cyChar = 16;
    int btnWidth  = (60 * cxChar) / 4;
    int btnHeight = (14 * cyChar) / 8;
    int bx = (m_TableRect.GetRight() / 2) - (btnWidth / 2);
    int by = m_TableRect.GetBottom() - card::dyCrd - (2 * POPSPACING) - btnHeight;
    m_btnLogicalPos  = wxPoint(bx, by);
    m_btnLogicalSize = wxSize(btnWidth, btnHeight);

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

    // Transform window-pixel mouse coordinates into the game's logical
    // 540x600 coordinate system.  Without this, clicks on scaled cards
    // would miss -- the player/card hit-testing code expects coords in
    // the same space the cards were laid out in (m_TableRect).
    wxPoint L = WindowToLogical(wxPoint(event.GetX(), event.GetY()));

    // Custom-drawn pass button: hit-test first so a click on the button
    // fires the same wxEVT_BUTTON(IDM_BUTTON) event the native wxButton
    // used to, keeping the existing OnPass handler unchanged.  Must run
    // before card hit-testing or the button area (which overlaps the
    // table) would swallow the click as a card.
    if (PassButtonHitTest(L))
    {
        wxCommandEvent evt(wxEVT_BUTTON, IDM_BUTTON);
        evt.SetEventObject(this);
        GetEventHandler()->ProcessEvent(evt);
        return;
    }

    if (mode == SELECTING)
    {
        p0->PopCard(m_BgndBrush, L.x, L.y);
        return;
    }
    else if (mode != PLAYING)
        return;

    p0->SetMode(WAITING);
    if (p0->PlayCard(L.x, L.y, handinfo, bCheating))
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

void CMainWindow::RenderScene(wxDC &dc)
{
    // Paint the green table background.  With wxBG_STYLE_PAINT the
    // system will not erase the background for us, so we have to do
    // it ourselves here.  Fill at the logical size (WINWIDTH x
    // WINHEIGHT) -- that's the size of the backbuffer we render into.
    // The surrounding letterbox bars in the actual window are painted
    // green directly in OnPaint.
    dc.SetBrush(m_BgndBrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, WINWIDTH, WINHEIGHT);

    // players must be painted in order starting with playerled so that
    // cards in centre overlap correctly

    if (bConstructed)
    {
        // If a card has been led this trick, start drawing with the
        // leader so later plays overlap on top.  Otherwise (pass-
        // selection phase, pre-game, or between tricks before the
        // next lead is set) playerled is -1; fall back to drawing
        // in natural pos order so the hand is still rendered.  The
        // previous `if (start >= 0)` guard skipped drawing entirely
        // in that case, which caused a resize during pass-selection
        // to blit an empty (green-only) backbuffer over the window.
        int start = (handinfo.playerled >= 0)
                    ? Id2Pos(handinfo.playerled) : 0;

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

    DrawPassButton(dc);
}


CMainWindow::ScaleInfo CMainWindow::GetScaleInfo() const
{
    // Uniform scale that maps the fixed 540x600 logical backbuffer to
    // the largest aspect-preserving rectangle that fits in the current
    // client area, centred -- the remaining area becomes green
    // letterbox/pillarbox bars filled by OnPaint.  All game positions,
    // m_TableRect, card coordinates, etc. stay in the logical
    // coordinate system; we only transform at the window boundary
    // (paint / mouse / RefreshRect).
    wxSize sz = GetClientSize();
    int cw = sz.GetWidth()  > 0 ? sz.GetWidth()  : WINWIDTH;
    int ch = sz.GetHeight() > 0 ? sz.GetHeight() : WINHEIGHT;
    double sxRaw = (double)cw / (double)WINWIDTH;
    double syRaw = (double)ch / (double)WINHEIGHT;
    double s = sxRaw < syRaw ? sxRaw : syRaw;
    if (s <= 0.0) s = 1.0;
    int rw = (int)(WINWIDTH  * s + 0.5);
    int rh = (int)(WINHEIGHT * s + 0.5);
    ScaleInfo info;
    info.sx      = s;
    info.sy      = s;
    info.renderW = rw;
    info.renderH = rh;
    info.offsetX = (cw - rw) / 2;
    info.offsetY = (ch - rh) / 2;
    return info;
}


wxPoint CMainWindow::WindowToLogical(const wxPoint &p) const
{
    ScaleInfo si = GetScaleInfo();
    if (si.sx == 0.0 || si.sy == 0.0)
        return p;
    return wxPoint(
        (int)((p.x - si.offsetX) / si.sx),
        (int)((p.y - si.offsetY) / si.sy));
}


void CMainWindow::RefreshLogicalRect(const wxRect &logical, bool eraseBg)
{
    // Transforms a logical-coord rect to the corresponding window-coord
    // rect (scaled + letterbox-offset) and invalidates it.  Callers
    // throughout the game (card animation, pop-card, bad-move timer,
    // etc.) work in logical coords; passing those rects straight to
    // wxWindow::RefreshRect would under-invalidate the stretched
    // window region and leave stale pixels visible.
    ScaleInfo si = GetScaleInfo();
    // Pad by 1 pixel on each side to absorb rounding when sx/sy are
    // non-integer (e.g. user has dragged to a size that doesn't give
    // a whole-number scale factor).
    wxRect w(
        (int)(logical.x      * si.sx) + si.offsetX - 1,
        (int)(logical.y      * si.sy) + si.offsetY - 1,
        (int)(logical.width  * si.sx) + 3,
        (int)(logical.height * si.sy) + 3);
    RefreshRect(w, eraseBg);
}


void CMainWindow::OnSize(wxSizeEvent &event)
{
    // The pass button is now drawn into the backbuffer in RenderScene,
    // not a native wxWindow child, so no per-frame SetSize is needed
    // -- the button scales automatically through the StretchBlit.
    Refresh(false);
    event.Skip();
}


// Draw the pass button directly into the backbuffer.  Rendered in
// logical coords; StretchBlit in OnPaint scales it uniformly with
// the rest of the scene.
void CMainWindow::DrawPassButton(wxDC &dc) const
{
    if (!m_bPassBtnVisible)
        return;

    wxRect r(m_btnLogicalPos, m_btnLogicalSize);

    // Button face: light grey if enabled, darker grey if disabled.
    // Border: black outline, one inset highlight/shadow line for a
    // classic Win3.1-ish push-button look.
    wxColour face    = m_bPassBtnEnabled ? wxColour(211, 211, 211)
                                         : wxColour(160, 160, 160);
    wxColour hilite  = m_bPassBtnEnabled ? wxColour(255, 255, 255)
                                         : wxColour(200, 200, 200);
    wxColour shadow  = wxColour(100, 100, 100);
    wxColour border  = wxColour(  0,   0,   0);

    dc.SetPen(wxPen(border, 1));
    dc.SetBrush(wxBrush(face));
    dc.DrawRectangle(r);

    // 3D edge (top-left hi, bottom-right lo), adjusted if pressed.
    wxColour tl = m_bPassBtnPressed ? shadow : hilite;
    wxColour br = m_bPassBtnPressed ? hilite : shadow;
    dc.SetPen(wxPen(tl, 1));
    dc.DrawLine(r.GetLeft()+1,  r.GetTop()+1,    r.GetRight(),   r.GetTop()+1);
    dc.DrawLine(r.GetLeft()+1,  r.GetTop()+1,    r.GetLeft()+1,  r.GetBottom());
    dc.SetPen(wxPen(br, 1));
    dc.DrawLine(r.GetLeft()+1,  r.GetBottom()-1, r.GetRight()-1, r.GetBottom()-1);
    dc.DrawLine(r.GetRight()-1, r.GetTop()+1,    r.GetRight()-1, r.GetBottom());

    // Label, centered.
    wxFont oldFont = dc.GetFont();
    wxFont btnFont(wxFontInfo(10).Family(wxFONTFAMILY_SWISS));
    dc.SetFont(btnFont);
    dc.SetTextBackground(face);
    dc.SetBackgroundMode(wxTRANSPARENT);
    dc.SetTextForeground(m_bPassBtnEnabled ? wxColour(0,0,0)
                                           : wxColour(128,128,128));
    wxCoord tw, th;
    dc.GetTextExtent(m_passBtnLabel, &tw, &th);
    int tx = r.GetLeft() + (r.GetWidth()  - tw) / 2;
    int ty = r.GetTop()  + (r.GetHeight() - th) / 2;
    if (m_bPassBtnPressed) { tx += 1; ty += 1; }
    dc.DrawText(m_passBtnLabel, tx, ty);
    dc.SetFont(oldFont);
}


bool CMainWindow::PassButtonHitTest(const wxPoint &logical) const
{
    if (!m_bPassBtnVisible || !m_bPassBtnEnabled)
        return false;
    wxRect r(m_btnLogicalPos, m_btnLogicalSize);
    return r.Contains(logical);
}


void CMainWindow::PassBtnShow()
{
    if (m_bPassBtnVisible) return;
    m_bPassBtnVisible = true;
    wxRect r(m_btnLogicalPos, m_btnLogicalSize);
    RefreshLogicalRect(r, false);
}

void CMainWindow::PassBtnHide()
{
    if (!m_bPassBtnVisible) return;
    m_bPassBtnVisible = false;
    m_bPassBtnEnabled = false;
    m_bPassBtnPressed = false;
    wxRect r(m_btnLogicalPos, m_btnLogicalSize);
    RefreshLogicalRect(r, false);
}

void CMainWindow::PassBtnEnable(bool b)
{
    if (m_bPassBtnEnabled == b) return;
    m_bPassBtnEnabled = b;
    if (m_bPassBtnVisible)
    {
        wxRect r(m_btnLogicalPos, m_btnLogicalSize);
        RefreshLogicalRect(r, false);
    }
}

void CMainWindow::PassBtnSetLabel(const wxString &s)
{
    if (m_passBtnLabel == s) return;
    m_passBtnLabel = s;
    if (m_bPassBtnVisible)
    {
        wxRect r(m_btnLogicalPos, m_btnLogicalSize);
        RefreshLogicalRect(r, false);
    }
}


void CMainWindow::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);
    wxSize sz = GetClientSize();

    // Paint the entire client area green first.  This both clears the
    // letterbox/pillarbox bars around the scaled game area and handles
    // the case where the backbuffer isn't yet allocated.
    dc.SetBrush(m_BgndBrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, sz.GetWidth(), sz.GetHeight());

    if (!m_backbuffer.IsOk())
    {
        // Degenerate fallback; shouldn't normally happen.
        RenderScene(dc);
        return;
    }

    // Outside of animation, refresh the backbuffer from the current
    // game state.  During animation, leave the backbuffer alone --
    // card::GlideStep writes each animation frame directly into it,
    // and re-rendering the scene here would overwrite those frames
    // with cards at their stale logical positions (the "screen stays
    // green" / "screen goes white" symptoms seen on wxGTK3).
    if (!bAnimating)
    {
        wxMemoryDC mdc;
        mdc.SelectObject(m_backbuffer);
        RenderScene(mdc);
        mdc.SelectObject(wxNullBitmap);
    }

    // StretchBlit the fixed-size logical backbuffer to the scaled
    // target rectangle (letterboxed/pillarboxed, centred).  This is
    // the single place where logical coords become window coords for
    // drawing; the surrounding green fill above covers the bars.
    ScaleInfo si = GetScaleInfo();
    wxMemoryDC mdc;
    mdc.SelectObject(m_backbuffer);
    int bw = m_backbuffer.GetWidth();
    int bh = m_backbuffer.GetHeight();
    dc.StretchBlit(si.offsetX, si.offsetY, si.renderW, si.renderH,
                   &mdc, 0, 0, bw, bh);
    mdc.SelectObject(wxNullBitmap);
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
        PassBtnHide();
        PassBtnSetLabel(wxT(""));
        p[0]->SetMode(WAITING);             // local human pushed the button

        wxRect rect;
        p[0]->GetCoverRect(rect);

        for (SLOT s = 0; s < MAXSLOT; s++)
            p[0]->Select(s, false);

        RefreshLogicalRect(rect, true);
        Update();

        FirstMove();

        for (int i = 0; i < ::cQdMoves; i++)
            HandleMove(::moveq[i]);

        ::cQdMoves = 0;
        ::cQdPasses = 0;

        return;
    }

    PassBtnEnable(false);
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
        RefreshLogicalRect(rect, true);
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
    PassBtnEnable(true);
}

void CMainWindow::OnHideButton(wxCommandEvent &event)
{
    PassBtnEnable(false);
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
    RefreshLogicalRect(m_badMoveRect, false);
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
        RefreshLogicalRect(m_badMoveRect, false);
    }
}


/****************************************************************************

CMainWindow::OnDispatchTimer

****************************************************************************/

void CMainWindow::OnDispatchTimer(wxTimerEvent &event)
{
    DispatchCards();
}
