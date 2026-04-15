#ifndef MAIN_INC
#define MAIN_INC

#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include "regentry.h"
#include "player.h"
#include "computer.h"
#include "dlg.h"

// Registry path for settings
extern const wxString szRegPath;
extern const wxString regvalSound;
extern const wxString regvalName;
extern const wxString regvalRole;
extern const wxString regvalSpeed;
extern const wxString regvalServer;
extern const wxString regvalPName[3];

const int WINWIDTH  = 540;
const int WINHEIGHT = 600;

const int LEFT_DIR  = 0;    // passdir values (renamed from LEFT to avoid conflict)
const int RIGHT_DIR = 1;
const int ACROSS    = 2;
const int NOPASS    = 3;

const int OFF       = 0;

const int MAXNAMELENGTH   = 14;
const int MAXCOMPUTERNAME = 15;

enum roletype { GAMEMEISTER, PLAYER };

// Data structures for game state (originally used for DDE, now just internal)
typedef struct {
    int     id;
    wxChar  name[4][MAXNAMELENGTH+3];
} GAMESTATUS;

typedef struct {
    int     id;
    int     passdir;
    int     cardid[3];
} PASS3;

typedef struct {
    int     passdir;
    int     cardid[MAXPLAYER][3];
} PASS12;

typedef struct {
    int     playerid;
    int     cardid;
    int     playerled;
    int     turn;
} MOVE;

class CMainWindow : public wxFrame
{
    wxDECLARE_EVENT_TABLE();

    friend class player;

public:
    CMainWindow();

    void     FatalError(int errorno = -1);
    int      GetGameNumber()            { return m_gamenumber; }
    wxColour GetBkColor()               { return m_bkgndcolor; }
    wxString GetPlayerName(int num)     { return p[num]->GetName(); }
    modetype GetPlayerMode(int num)     { return p[num]->GetMode(); }
    int      GetMyId()                  { return m_myid; }
    int      Id2Pos(int id)             { return ((id - m_myid + 4) % 4); }
    bool     IsFirstBloodEnforced()     { return bEnforceFirstBlood; }
    void     PlayerQuit(int id);
    int      Pos2Id(int pos)            { return ((pos + m_myid) % 4); }
    void     SetGameNumber(int num)     { m_gamenumber = num; }

    void StartBadMoveTimer(const wxRect &rect);

    void OnAbout(wxCommandEvent &event);
    void OnChar(wxKeyEvent &event);
    void OnCheat(wxCommandEvent &event);
    void OnClose(wxCloseEvent &event);
    void OnEraseBkgnd(wxEraseEvent &event);
    void OnLeftDown(wxMouseEvent &event);
    void OnNewGame(wxCommandEvent &event);
    void OnOptions(wxCommandEvent &event);
    void OnPaint(wxPaintEvent &event);
    void OnPass(wxCommandEvent &event);
    void OnQuote(wxCommandEvent &event);
    void OnRef(wxCommandEvent &event);
    void OnRef();  // direct call version
    void OnScore(wxCommandEvent &event);
    void OnSound(wxCommandEvent &event);
    void OnWelcome(wxCommandEvent &event);
    void OnShowButton(wxCommandEvent &event);
    void OnHideButton(wxCommandEvent &event);
    void OnBossKey(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnBadMoveTimer(wxTimerEvent &event);

    void DispatchCards();

    static wxBrush  m_BgndBrush;
    static wxRect   m_TableRect;

public:
    bool     bAnimating;

    // Persistent backbuffer.  All game drawing ultimately ends up in
    // this bitmap, and OnPaint simply blits it to the window.  This
    // decouples what's on screen from paint-event timing, which is
    // what wxGTK3 needs -- wxClientDC writes there are not reliably
    // committed to the window's backing store, so without a backbuffer
    // Refresh()/Update() during animation would leave the window blank
    // (or repainted green on top of the animation frame).
    wxBitmap m_backbuffer;

    // Render the current scene (green table + cards in their logical
    // positions) into the supplied DC.  Used by OnPaint to refresh the
    // backbuffer outside of animation, and callable from elsewhere to
    // force a full re-render.
    void     RenderScene(wxDC &dc);

protected:
    wxButton *m_pButton;
    int      m_StatusHeight;
    CScoreDlg *m_pScoreDlg;
    wxTimer  m_badMoveTimer;
    wxTimer  m_dispatchTimer;
    wxRect   m_badMoveRect;

    bool     bAutostarted;
    bool     bCheating;
    bool     bConstructed;
    bool     bEnforceFirstBlood;
    bool     bHasSound;
    bool     bNetDdeActive;
    bool     bSoundOn;
    bool     bTimerOn;
    int      m_gamenumber;
    player   *p[MAXPLAYER];
    int      m_myid;
    int      passdir;
    int      m_FatalErrno;

    wxColour        m_bkgndcolor;
    handinfotype    handinfo;
    roletype        role;

    int      tricksleft;
    int      trickwinner;

    // Game logic methods (from main2.cpp)
    void     DoSort();
    void     EndHand();
    void     FirstMove();
    void     GameOver();
    void     HandleMove(MOVE &move);
    void     HandlePass(PASS3 &pass3);
    bool     HandlePassing();
    void     ResetHandInfo(int playernumber);
    void     Shuffle();
    bool     SoundInit();
    bool     HeartsPlaySound(int id);

    void     OnDispatchTimer(wxTimerEvent &event);
};

// Global variables
extern CMainWindow *pMainWnd;
extern MOVE    move;
extern MOVE    moveq[8];
extern int     cQdMoves;
extern PASS3   passq[4];
extern int     cQdPasses;
extern int     nStatusHeight;

#endif
