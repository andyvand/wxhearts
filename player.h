/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

player.h -- wxWidgets port

Header file for class player

hierarchy:              player
                      /       \
                 computer     human
                             /     \
                   local_human     remote_human

note: player and human are abstract classes.

    pos == 0 implies local human
    id  == 0 implies gamemeister

Relative to any human player, positions (pos) are arranged like this:

        2
      1   3
        0

****************************************************************************/

#ifndef PLAYER_INC
#define PLAYER_INC

#include "card.h"
#include "debug.h"

const int HORZSPACING = 15;
const int VERTSPACING = 15;
const int IDGE        = 3;
const int MAXCARDSWON = 14;

typedef int SLOT;

enum modetype { STARTING,
                SELECTING,
                DONE_SELECTING,
                WAITING,
                ACCEPTING,
                PLAYING,
                SCORING
              };

const int MAXSLOT     = 13;
const int ALL         = -1;
const int MAXPLAYER   = 4;

struct handinfotype {
    int     playerled;
    int     turn;
    card    *cardplayed[4];
    bool    bHeartsBroken;
    bool    bQSPlayed;
    bool    bShootingRisk;
    int     nMoonShooter;
    bool    bHumanShooter;
};

class CMainWindow;

class player {

    private:
        wxString    name;
        wxFont      font;

    protected:
        int         id;
        int         position;
        int         score;
        card        cd[MAXSLOT];
        wxPoint     loc;
        int         dx, dy;
        wxPoint     playloc;
        wxPoint     homeloc;
        wxPoint     dotloc;
        wxPoint     nameloc;
        modetype    mode;
        int         status;

        int         cardswon[MAXCARDSWON];
        int         numcardswon;

    public:
        player(int n, int pos);
        virtual ~player() { }

        card    *Card(int s) { return &(cd[s]); }
        void    DisplayHeartsWon(wxDC &dc);
        void    DisplayName(wxDC &dc, const wxRect *cardBounds = nullptr);
        int     EvaluateScore(bool &bMoonShot);
        bool    GetCardLoc(SLOT s, wxPoint &loc);
        SLOT    GetSlot(int id);
        wxRect  &GetCoverRect(wxRect &rect);
        int     GetID(SLOT slot) { return cd[slot].ID(); }
        wxRect  &GetMarkingRect(wxRect &rect);
        modetype GetMode() { return mode; }
        wxString GetName() { return name; }
        int     GetScore() { return score; }
        void    GlideToCentre(SLOT s, bool bFaceup);
        void    MarkCardPlayed(SLOT s)      { cd[s].Play(); }
        void    ResetCardsWon();
        void    ResetLoc();
        void    ReturnSelectedCards(int c[]);
        void    Select(SLOT slot, bool bSelect) { cd[slot].Select(bSelect); }
        void    SetID(SLOT slot, int id) { cd[slot].SetID(id); }
        void    SetMode(modetype m) { mode = m; }
        void    SetName(wxString &newname, wxDC &dc);
        void    SetScore(int s)     { score = s; }
        void    SetStatus(int s)    { status = s; }
        void    Sort();
        void    WinCard(wxDC &dc, card *c);

        virtual void Draw(wxDC &dc, bool bCheating = false, SLOT slot = ALL);
        virtual bool IsHuman()      { return false; }
        virtual void MarkSelectedCards(wxDC &dc);
        virtual void NotifyEndHand(handinfotype &h) = 0;
        virtual void NotifyNewRound() = 0;

        virtual void ReceiveSelectedCards(int c[]);
        virtual void SelectCardsToPass() = 0;
        virtual void SelectCardToPlay(handinfotype &h, bool bCheating) = 0;

        virtual void UpdateStatus() = 0;
        virtual void UpdateStatus(int stringid) = 0;
        virtual void UpdateStatus(const wxString &string) = 0;

        virtual void Quit()     { }
        virtual bool HasQuit()  { return false; }
};

class human : public player {

    protected:
        human(int n, int pos);

    public:
        virtual bool IsHuman()  { return true; }
};

class remote_human : public human {

    private:
        bool    bQuit;

    public:
        remote_human(int n, int pos);

        virtual void NotifyEndHand(handinfotype &h) { }
        virtual void NotifyNewRound() { }
        virtual void SelectCardsToPass();
        virtual void SelectCardToPlay(handinfotype &h, bool bCheating);
        virtual void UpdateStatus()  { }
        virtual void UpdateStatus(int stringid) { status = stringid; }
        virtual void UpdateStatus(const wxString &string) { }
        virtual void Quit()         { bQuit = true; }
        virtual bool HasQuit()      { return bQuit; }
};

class local_human : public human {

    protected:
        wxBitmap    m_bmStretchCard;

        int     XYToCard(int x, int y);
        void    StartTimer(card &c);

        static bool     bTimerOn;
        static wxString m_StatusText;

    public:
        local_human(int n);
        ~local_human();

        bool IsTimerOn() { return bTimerOn; }
        static void ResetTimer() { bTimerOn = false; }
        bool PlayCard(int x, int y, handinfotype &h, bool bCheating,
                      bool bFlash = true);
        void PopCard(wxBrush &brush, int x, int y);
        void SetPlayerId(int n)     { id = n; }
        void WaitMessage(const wxString &name);

        virtual void Draw(wxDC &dc, bool bCheating = false, SLOT slot = ALL);
        virtual void MarkSelectedCards(wxDC &dc) { }
        virtual void NotifyEndHand(handinfotype &h) { }
        virtual void NotifyNewRound() { }
        virtual void ReceiveSelectedCards(int c[]);
        virtual void SelectCardsToPass();
        virtual void SelectCardToPlay(handinfotype &h, bool bCheating);
        virtual void UpdateStatus();
        virtual void UpdateStatus(int stringid);
        virtual void UpdateStatus(const wxString &string);
};

#endif
