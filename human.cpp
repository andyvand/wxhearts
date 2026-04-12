/* human.cpp -- wxWidgets port of local_human and remote_human */
#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"
#include <cstdio>
#include <cstdlib>

static wxRect rectCard;
extern CMainWindow *pMainWnd;

bool     local_human::bTimerOn;
wxString local_human::m_StatusText;


/* human constructor -- abstract class */

human::human(int n, int pos) : player(n, pos)
{
}


/* remote_human constructor */

remote_human::remote_human(int n, int pos) : human(n, pos),
                            bQuit(false)
{
}


/*
 * remote_human::SelectCardToPlay()
 *
 * Under normal circumstances, all that is required is that mode be set
 * to PLAYING.  If the remote human has quit and the computer player has
 * not yet taken over, this routine just picks the first legal card it
 * can find and plays it.
 */

void remote_human::SelectCardToPlay(handinfotype &h, bool bCheating)
{
    if (!bQuit)
    {
        SetMode(PLAYING);
        return;
    }

    bool bFirst   = (h.playerled == id);            // am I leading?
    card *cardled = h.cardplayed[h.playerled];
    int  nSuitLed = (cardled == NULL ? EMPTY : cardled->Suit());

    SLOT sLast[MAXSUIT];                // will contain some card of each suit
    SLOT s = EMPTY;

    for (int i = 0; i < MAXSUIT; i++)
        sLast[i] = EMPTY;

    // fill sLast array, and look for two of clubs while we are at it

    for (int i = 0; i < MAXSLOT; i++)
    {
        if (cd[i].IsValid())
        {
            sLast[cd[i].Suit()] = i;
            if (cd[i].ID() == TWOCLUBS)
                s = i;
        }
    }

    if (s == EMPTY)                         // if two of clubs not found
    {
        if (sLast[CLUBS] != EMPTY)
            s = sLast[CLUBS];
        else if (sLast[DIAMONDS] != EMPTY)
            s = sLast[DIAMONDS];
        else if (sLast[SPADES] != EMPTY)
            s = sLast[SPADES];
        else
            s = sLast[HEARTS];

        if (!bFirst && (sLast[nSuitLed] != EMPTY))
            s = sLast[nSuitLed];
    }

    SetMode(WAITING);
    cd[s].Play();                                   // mark card as played
    h.cardplayed[id] = &(cd[s]);                    // update handinfo

    // inform other players (local-only mode, no DDE)

    ::move.playerid = id;
    ::move.cardid = cd[s].ID();
    ::move.playerled = h.playerled;
    ::move.turn = h.turn;

    // In the MFC version, ddeServer->PostAdvise(hszMove) was called here.
    // In the wxWidgets local-only port, we skip the DDE notification.

    // inform gamemeister
    wxCommandEvent evt(wxEVT_MENU, IDM_REF);
    pMainWnd->GetEventHandler()->QueueEvent(evt.Clone());
}


/*
 * remote_human::SelectCardsToPass()
 *
 * Under normal circumstances, all that is required is that mode be set to
 * SELECTING.  If the remote human has quit and the computer player has not
 * yet taken over, just select the first three cards found.
 */

void remote_human::SelectCardsToPass()
{
    if (!bQuit)
    {
        SetMode(SELECTING);
        return;
    }

    cd[0].Select(true);
    cd[1].Select(true);
    cd[2].Select(true);

    for (int i = 3; i < MAXSLOT; i++)
        cd[i].Select(false);

    wxClientDC dc(pMainWnd);
    MarkSelectedCards(dc);

    SetMode(DONE_SELECTING);

    // In the MFC version, ddeServer->PostAdvise(hszPass) was called here.
    // In the wxWidgets local-only port, we skip the DDE notification.
}


/*
 * local_human::local_human()
 *
 * This is the constructor that initializes the local human player.
 * It also creates the stretch bitmap that covers a card plus its popped
 * height extension.
 */

local_human::local_human(int n) : human(n, 0)
{
    m_StatusText = GetStringResource(IDS_INTRO);

    wxClientDC dc(pMainWnd);

    UpdateStatus();

    bTimerOn = false;

    m_bmStretchCard = wxBitmap(card::dxCrd, card::dyCrd + POPSPACING);
    if (!m_bmStretchCard.IsOk())
    {
        pMainWnd->FatalError(IDS_MEMORY);
        return;
    }
}


/* local_human destructor */

local_human::~local_human()
{
    m_bmStretchCard = wxNullBitmap;
}


/*
 * local_human::Draw()
 *
 * This virtual function draws selected cards in the popped up position.
 * ALL is not used for slot in this variant.
 */

void local_human::Draw(wxDC &dc, bool bCheating, SLOT slot)
{
    DisplayName(dc);
    SLOT start = (slot == ALL ? 0 : slot);
    SLOT stop  = (slot == ALL ? MAXSLOT : slot+1);

    SLOT playedslot = EMPTY;            // must draw cards in play last for EGA

    for (SLOT s = start; s < stop; s++)
    {
        if (cd[s].IsPlayed())
            playedslot = s;
        else
            cd[s].PopDraw(dc);          // pop up selected cards
    }

    if (playedslot != EMPTY)
        cd[playedslot].Draw(dc);
}


/*
 * local_human::PopCard()
 *
 * handles mouse button selection of card to pass
 */

void local_human::PopCard(wxBrush &brush, int x, int y)
{
    SLOT s = XYToCard(x, y);
    if (s == EMPTY)
        return;

    // count selected cards

    int c = 0;
    for (int i = 0; i < MAXSLOT; i++)
        if (cd[i].IsSelected())
            c++;

    if (cd[s].IsSelected() && (c == 3))
    {
        wxCommandEvent evt(wxEVT_MENU, IDM_HIDEBUTTON);
        pMainWnd->GetEventHandler()->QueueEvent(evt.Clone());
    }
    else if (!cd[s].IsSelected())
    {
        if (c == 3)                 // only allow three selections
            return;
        else if (c == 2)
        {
            wxCommandEvent evt(wxEVT_MENU, IDM_SHOWBUTTON);
            pMainWnd->GetEventHandler()->QueueEvent(evt.Clone());
        }
    }

    // toggle selection

    bool bSelected = cd[s].IsSelected();
    cd[s].Select(!bSelected);

    wxClientDC dc(pMainWnd);
    wxMemoryDC memDC;
    memDC.SelectObject(m_bmStretchCard);
    memDC.SetBrush(brush);
    memDC.SetPen(*wxTRANSPARENT_PEN);
    memDC.DrawRectangle(0, 0, card::dxCrd, card::dyCrd + POPSPACING);

    for (int i = 0; i < MAXSLOT; i++)
    {
        if (abs(i - s) <= (card::dxCrd / HORZSPACING))
        {
            cd[i].Draw(memDC,                                   // dc
                       (i - s) * HORZSPACING,                   // x
                       cd[i].IsSelected() ? 0 : POPSPACING,     // y
                       FACEUP,                                  // mode
                       false);                                  // update loc?
        }
    }
    dc.Blit(loc.x + (HORZSPACING * s), loc.y - POPSPACING,
           card::dxCrd, card::dyCrd + POPSPACING,
           &memDC, 0, 0);
    memDC.SelectObject(wxNullBitmap);

    // On Linux/GTK3, wxClientDC drawing is unreliable.  Force a
    // repaint through OnPaint so PopDraw renders the offset correctly.
    pMainWnd->Refresh();
    pMainWnd->Update();
}


/*
 * local_human::PlayCard()
 *
 * handles mouse button selection of card to play
 * and ensures move is legal.
 *
 * PlayCard starts a timer that calls StartTimer().
 * Think of it as one long function with a timer delay half way through.
 */

bool local_human::PlayCard(int x, int y, handinfotype &h, bool bCheating,
                            bool bFlash)
{
    SLOT s = XYToCard(x, y);
    if (s == EMPTY)
        return false;

    card *cardled    = h.cardplayed[h.playerled];
    bool bFirstTrick = (cardled != NULL && cardled->ID() == TWOCLUBS);

    /* check if selected card is valid */

    if (h.playerled == id)              // if local human is leading...
    {
        if (cd[s].ID() != TWOCLUBS)
        {
            for (int i = 0; i < MAXSLOT; i++)   // is there a two of clubs?
            {
                if ((i != s) && (cd[i].ID() == TWOCLUBS))
                {
                    UpdateStatus(IDS_LEAD2C);
                    if (bFlash)
                        StartTimer(cd[s]);

                    return false;
                }
            }
        }
        if ((cd[s].Suit() == HEARTS) && (!h.bHeartsBroken))   // if hearts led
        {
            for (int i = 0; i < MAXSLOT; i++)   // are there any non-hearts?
            {
                if ((!cd[i].IsEmpty()) && (cd[i].Suit() != HEARTS))
                {
                    UpdateStatus(IDS_LEADHEARTS);
                    if (bFlash)
                        StartTimer(cd[s]);

                    return false;
                }
            }
        }
    }

    // if not following suit

    else if (cardled != NULL && (cd[s].Suit() != cardled->Suit()))
    {
        // make sure we're following suit if possible

        for (int i = 0; i < MAXSLOT; i++)
        {
            if ((!cd[i].IsEmpty()) && (cd[i].Suit()==cardled->Suit()))
            {
                wxString s1, s2, sSpace, sDot, string;
                s1 = GetStringResource(IDS_BADMOVE);
                s2 = GetStringResource(IDS_SUIT0+cardled->Suit());
                sSpace = wxT(" ");
                sDot = wxT(".");
                string = s1 + sSpace + s2 + sDot;

                if (bFlash)
                {
                    UpdateStatus(string);
                    StartTimer(cd[s]);
                }

                return false;
            }
        }

        // make sure we're not trying to break the First Blood rule

        if (bFirstTrick && pMainWnd->IsFirstBloodEnforced())
        {
            bool bPointCard =
                         (cd[s].Suit() == HEARTS || cd[s].ID() == BLACKLADY);

            bool bOthersAvailable = false;

            for (int i = 0; i < MAXSLOT; i++)
                if ((!cd[i].IsEmpty()) && (cd[i].Suit() != HEARTS))
                    if (cd[i].ID() != BLACKLADY)
                        bOthersAvailable = true;

            if (bPointCard && bOthersAvailable)
            {
                UpdateStatus(IDS_BADBLOOD);
                if (bFlash)
                    StartTimer(cd[s]);

                return false;
            }
        }
    }

    SetMode(WAITING);
    cd[s].Play();
    h.cardplayed[id] = &(cd[s]);

    ::move.playerid = id;
    ::move.cardid = cd[s].ID();
    ::move.playerled = h.playerled;
    ::move.turn = h.turn;

    // In the MFC version, DDE was used here to notify other players.
    // In the wxWidgets local-only port, we skip DDE calls.

    pMainWnd->OnRef();

    return true;
}


void local_human::StartTimer(card &c)
{
    wxClientDC dc(pMainWnd);
    c.Draw(dc, HILITE);           // flash
    c.GetRect(rectCard);

    pMainWnd->StartBadMoveTimer(rectCard);
    bTimerOn = true;
}


/*
 * local_human::XYToCard()
 *
 * returns a card slot number (or EMPTY) given a mouse location
 */

int local_human::XYToCard(int x, int y)
{
    // check that we are in the right general area on the screen

    if (y < (loc.y - POPSPACING))
        return EMPTY;

    if (y > (loc.y + card::dyCrd))
        return EMPTY;

    if (x < loc.x)
        return EMPTY;

    if (x > (loc.x + (12 * HORZSPACING) + card::dxCrd))
        return EMPTY;

    // Take first stab at card selected.

    SLOT s = (x - loc.x) / HORZSPACING;
    if (s > 12)
        s = 12;

    // If the click is ABOVE the top of the normal card location,
    // check to see if this is a selected card.

    if (y < loc.y)
    {
        // If the card is selected, then we have it.  If not, it could
        // be overhanging other cards.

        if (!cd[s].IsSelected())
        {
            for (;;)
            {
                if (s == 0)
                    return EMPTY;
                s--;

                // if this card doesn't extend as far as x, give up

                if ((loc.x + (s * HORZSPACING) + card::dxCrd) < x)
                    return EMPTY;

                // if this card is selected, we've got it

                if (cd[s].IsSelected())
                    break;
            }
        }
    }

    // a similar check is used to make sure we pick a card not yet played

    if (!cd[s].IsInHand())
    {
        for (;;)
        {
            if (s == 0)
                return EMPTY;
            s--;

            // if this card doesn't extend as far as x, give up

            if ((loc.x + (s * HORZSPACING) + card::dxCrd) < x)
                return EMPTY;

            // if this card is in hand, we've got it

            if (cd[s].IsInHand())
                break;
        }
    }

    return s;
}


/* local_human::SelectCardsToPass() */

void local_human::SelectCardsToPass()
{
    SetMode(SELECTING);
}


/* local_human::SelectCardToPlay */

void local_human::SelectCardToPlay(handinfotype &h, bool bCheating)
{
    SetMode(PLAYING);
    UpdateStatus(IDS_GO);
}


/*
 * local_human::UpdateStatus
 *
 * The status bar can be updated either by manually filling m_StatusText
 * or by passing a string resource id.
 */

void local_human::UpdateStatus(void)
{
    pMainWnd->SetStatusText(m_StatusText);
}

void local_human::UpdateStatus(int stringid)
{
    status = stringid;
    m_StatusText = GetStringResource(stringid);
    UpdateStatus();
}

void local_human::UpdateStatus(const wxString &string)
{
    m_StatusText = string;
    UpdateStatus();
}


/*
 * local_human::ReceiveSelectedCards
 *
 * The parameter c[] is an array of three cards being passed from another
 * player.
 */

void local_human::ReceiveSelectedCards(int c[])
{
    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
            cd[i].SetID(c[j++]);

        wxASSERT(i < MAXSLOT);
    }

    SetMode(ACCEPTING);
    UpdateStatus(IDS_ACCEPT);
}


/*
 * local_human::WaitMessage()
 *
 * Makes and shows the "Waiting for %s to move..." message
 */

void local_human::WaitMessage(const wxString &name)
{
    wxString s1 = GetStringResource(IDS_WAIT1);
    wxString s2 = name;
    wxString s3 = GetStringResource(IDS_WAIT2);

    wxString buf = s1 + s2 + s3;

    UpdateStatus(buf);
}
