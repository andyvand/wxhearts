/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

card.h -- wxWidgets port

Header file for class card.
Cards are drawn using wxBitmap loaded from bmp/ directory.

****************************************************************************/

#ifndef CARD_INC
#define CARD_INC

#include <wx/wx.h>

const int EMPTY       = -1;
const int FACEUP      = 0;
const int FACEDOWN    = 1;
const int HILITE      = 2;
const int CARDBACK    = 54;
const int ACE         = 0;
const int QUEEN       = 11;
const int KING        = 12;

const int TWOCLUBS    = 4;
const int BLACKLADY   = 47;
const int CLUBS       = 0;
const int DIAMONDS    = 1;
const int HEARTS      = 2;
const int SPADES      = 3;

const int POPSPACING  = 20;
const int MENU_HEIGHT = 0;
const int MAXSUIT     = 4;

enum statetype { NORMAL, SELECTED, PLAYED, HIDDEN };

class card {

    private:
        int         id;
        wxPoint     loc;
        statetype   state;

        static int  count;
        static int  stepsize;

        /* Static bitmaps for animation */
        static wxBitmap m_bmFgnd;
        static wxBitmap m_bmBgnd2;
        static wxBitmap m_bmCard[52];
        static wxBitmap m_bmBack;
        static wxBitmap m_bmGhost;
        static bool     m_bitmapsLoaded;

        void GlideStep(int x1, int y1, int x2, int y2);
        void SaveCorners(wxDC &dc, int x, int y);
        void RestoreCorners(wxDC &dc, int x, int y);
        int  IntSqrt(long square);

        static bool LoadCardBitmaps();

    public:
        static bool     bConstructed;
        static int      dxCrd, dyCrd;
        static wxBitmap m_bmBgnd;       // what's under card to glide

        card(int n = EMPTY);
        ~card();

        int  ID()     { return id; }
        int  Suit()   { return (id % MAXSUIT); }
        int  Value()  { return (id / MAXSUIT); }
        int  Value2() { int v = Value(); return ((v == ACE) ? (KING + 1) : v); }
        void Select(bool b) { state = (b ? SELECTED : NORMAL); }
        bool IsEmpty()    { return (id == EMPTY); }
        bool IsSelected() { return (state == SELECTED); }
        bool IsPlayed()   { return (state == PLAYED); }
        bool IsHeart()    { return (Suit() == HEARTS); }
        bool IsValid()    { if (id == EMPTY) return false; return true; }

        void SetID(int n) { id = n; }
        void SetLoc(int x, int y) { loc.x = x; loc.y = y; }
        int  SetStepSize(int s) { int old = stepsize; stepsize = s; return old; }
        int  GetX() { return loc.x; }
        int  GetY() { return loc.y; }

        bool Draw(wxDC &dc, int x, int y, int mode = FACEUP, bool bUpdateLoc = true);
        bool Draw(wxDC &dc) { return Draw(dc, loc.x, loc.y, FACEUP); }
        bool Draw(wxDC &dc, int mode) { return Draw(dc, loc.x, loc.y, mode); }
        bool PopDraw(wxDC &dc);
        bool CleanDraw(wxDC &dc);

        void Glide(wxDC &dc, int xEnd, int yEnd);
        wxRect &GetRect(wxRect &rect);
        void Remove() { state = HIDDEN; id = EMPTY; }
        void Play()   { state = PLAYED; }
        bool IsNormal() { return (state == NORMAL); }
        bool IsInHand() { return ((state == NORMAL) || (state == SELECTED)); }

        /* Static drawing function used by cards.cpp */
        static bool DrawCard(wxDC &dc, int x, int y, int dx, int dy,
                             int cd, int mode);
};

#endif
