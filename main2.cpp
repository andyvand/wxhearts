/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

main2.cpp -- wxWidgets port

Aug 92, JimH
May 93, JimH    chico port

Additional member functions for CMainWindow are here.

****************************************************************************/

#include "hearts.h"

#include "main.h"
#include "resource.h"
#include "debug.h"

#include <cstdlib>
#include <wx/sound.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

int score[MAXPLAYER];
/* nHandsPlayed is defined in dlg.cpp; declared in dlg.h via extern "C" */


/****************************************************************************

CMainWindow::Shuffle -- distributes cards to players

****************************************************************************/

void CMainWindow::Shuffle()
{
    static int offset[MAXPLAYER] = { 1, 3, 2, 0 };     // passdir order

    // fill temp array with consecutive values

    int temp[52];                   // array of card values
    for (int i = 0; i < 52; i++)
        temp[i] = i;

    //  Sort cards

    int nLeft = 52;
    for (int i = 0; i < 52; i++)
    {
        int j = rand() % nLeft;
        int id = i / 13;
        int pos = Id2Pos(id);               // convert id to position
        p[pos]->SetID(i % 13, temp[j]);
        p[pos]->Select(i % 13, false);
        temp[j] = temp[--nLeft];
    }

    // display PASS button

    if (passdir != NOPASS)
    {
        wxString text = GetStringResource(IDS_PASSLEFT + passdir);
        m_pButton->SetLabel(text);
        m_pButton->Enable(false);
        m_pButton->Show();
    }

    // set card locs and ask players to select cards to pass

    for (int i = 0; i < MAXPLAYER; i++)
    {
        p[i]->ResetLoc();

        if (passdir != NOPASS)
            p[i]->SelectCardsToPass();
    }

    //  Paint main window.  This is done manually instead of just
    //  invalidating the rectangle so that the cards are drawn in
    //  order as if they are dealt, instead of a player at a time.

    wxClientDC dc(this);
    wxSize clientSize = GetClientSize();
    dc.SetBrush(m_BgndBrush);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, clientSize.GetWidth(), clientSize.GetHeight());

    for (SLOT s = 0; s < MAXSLOT; s++)
        for (int i = 0; i < MAXPLAYER; i++)
            p[i]->Draw(dc, bCheating, s);

    for (int i = 0; i < MAXPLAYER; i++)
    {
        if (passdir == NOPASS)
            p[i]->NotifyNewRound();
        else
        {
            p[i]->MarkSelectedCards(dc);
            wxString sSelect = GetStringResource(IDS_SELECT);
            wxString sName;
            wxString sSpace = wxT(" ");
            wxString sDot = wxT(".");
            wxString sString;
            int passto = (i + offset[passdir]) % 4;
            sName = p[passto]->GetName();
            sString = sSelect + sSpace + sName + sDot;
            p[i]->UpdateStatus(sString);
        }
    }

    DoSort();
}


/****************************************************************************

CMainWindow::HandlePassing()

This function first checks to make sure each player is DONE_SELECTING,
and then transfers the cards from hand to hand.

This function is called by the gamemeister when he presses the pass
button, or when notification arrives that a remote human has selected
cards to pass.

It returns false if cards were not passed (because a remote human was
still selecting) and true if cards were successfully passed.

****************************************************************************/

bool CMainWindow::HandlePassing()
{
    int     passto[MAXPLAYER];
    int     temp[MAXPLAYER][3];

    static int offset[MAXPLAYER] = { 1, 3, 2, 0 };

    for (int pos = 0; pos < MAXPLAYER; pos++)
        if (p[pos]->GetMode() != DONE_SELECTING)
            return false;

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        passto[pos] = ((pos + offset[passdir]) % 4);
        p[pos]->ReturnSelectedCards(temp[pos]);
    }

    for (int pos = 0; pos < MAXPLAYER; pos++)
        p[passto[pos]]->ReceiveSelectedCards(temp[pos]);

    for (int pos = 0; pos < MAXPLAYER; pos++)
        if (bCheating || (pos == 0))
            p[pos]->Sort();

    tricksleft = MAXSLOT;

    passdir++;
    if (passdir > NOPASS)
        passdir = LEFT_DIR;

    for (int pos = 0; pos < MAXPLAYER; pos++)
        p[pos]->NotifyNewRound();           // notify players cards are passed

    wxString s = GetStringResource(IDS_OK);
    m_pButton->SetLabel(s);

    // OnShowButton equivalent
    if (m_pButton)
    {
        m_pButton->Enable(true);
        m_pButton->SetFocus();
    }

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        wxRect rect;

        if (pos == 0 || bCheating)
            p[pos]->GetCoverRect(rect);
        else
            p[pos]->GetMarkingRect(rect);

        RefreshRect(rect, true);
    }

    Update();
    return true;
}


/****************************************************************************

CMainWindow::FirstMove

resets cardswon[] and tells owner of two of clubs to start hand

****************************************************************************/

void CMainWindow::FirstMove()
{
    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        p[pos]->SetMode(WAITING);
        p[pos]->ResetCardsWon();
    }

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        for (SLOT s = 0; s < MAXSLOT; s++)
        {
            if (p[pos]->GetID(s) == TWOCLUBS)
            {
                int id = Pos2Id(pos);
                ResetHandInfo(id);
                handinfo.bHeartsBroken = false;
                handinfo.bQSPlayed = false;
                handinfo.bShootingRisk = true;
                handinfo.nMoonShooter = EMPTY;
                handinfo.bHumanShooter = false;
                p[pos]->SelectCardToPlay(handinfo, bCheating);

                if (pos != 0)
                    ((local_human *)p[0])->WaitMessage(p[pos]->GetName());

                return;
            }
        }
    }
}


/****************************************************************************

CMainWindow::EndHand

The Ref calls this routine at the end of each hand.  It is logically
a single routine, but is broken up so that there is a delay before the
cards are zipped off the screen.

EndHand() calculates who won the hand (trick) and starts a timer.

****************************************************************************/

void CMainWindow::EndHand()
{
    /* determine suit led */

    int  playerled = handinfo.playerled;
    card *cardled  = handinfo.cardplayed[playerled];
    int  suitled   = cardled->Suit();
    int  value     = cardled->Value2();

    trickwinner = playerled;               //  by default

    //  Let players update tables, etc.

    for (int i = 0; i < 4; i++)
        p[i]->NotifyEndHand(handinfo);

    // check if anyone else played a higher card of the same suit

    for (int i = playerled; i < (playerled + 4); i++)
    {
        int j = i % 4;
        card *c = handinfo.cardplayed[j];
        if (c->Suit() == suitled)
        {
            int v = c->Value2();

            if (v > value)
            {
                value = v;
                trickwinner = j;
            }
        }
    }

    TRACE0("\n");

    // Update moonshoot portion of handinfo

    if (handinfo.bShootingRisk)
    {
        bool bPoints = false;               // point cards this hand?

        for (int i = 0; i < 4; i++)
        {
            card *c = handinfo.cardplayed[i];
            if ((c->Suit() == HEARTS) || (c->ID() == BLACKLADY))
                bPoints = true;
        }

        if (bPoints)
        {
            if (handinfo.nMoonShooter == EMPTY)
            {
                handinfo.nMoonShooter = trickwinner;  // first points this round
                handinfo.bHumanShooter = p[trickwinner]->IsHuman();
            }
            else if (handinfo.nMoonShooter != trickwinner)   // new point earner
            {
                handinfo.bShootingRisk = false;
            }
        }
    }

    // Start a timer so there is a delay between when the last card of
    // the trick is played, and when the cards are whisked off toward
    // the trick winner (dispatched.)  If the timer fails, just call
    // DispatchCards() directly.

    if (m_dispatchTimer.StartOnce(1000))
        bTimerOn = true;
    else
    {
        bTimerOn = false;
        DispatchCards();
    }
}


/****************************************************************************

CMainWindow::DispatchCards

Called by the dispatch timer after the trick display delay.  Glides
cards towards the trick winner, then checks if there are more tricks
left or if the hand is over and scoring is needed.

****************************************************************************/

void CMainWindow::DispatchCards()
{
    m_dispatchTimer.Stop();

    bTimerOn = false;

    int poswinner = Id2Pos(trickwinner);

    // Determine who led so cards can be removed in reverse order.

    int  playerled = handinfo.playerled;
    card *cardled  = handinfo.cardplayed[playerled];

    // build up background bitmap for Glide()

    for (int i = (playerled + 3); i >= playerled; i--)
    {
        wxClientDC dc(this);

        wxBitmap bgBmp(card::dxCrd, card::dyCrd);
        {
            wxMemoryDC memdc;
            memdc.SelectObject(bgBmp);
            memdc.SetBrush(m_BgndBrush);
            memdc.SetPen(*wxTRANSPARENT_PEN);
            memdc.DrawRectangle(0, 0, card::dxCrd, card::dyCrd);

            card *c = handinfo.cardplayed[i % 4];

            // If cards overlap, there is some extra work to do because the cards
            // still in player 0's or 2's hands may overlap cards that have been
            // played, so they have to get blted in first.

            for (int pos = 0; pos < MAXPLAYER; pos += 2)
            {
                int mode = ((pos == 0 || bCheating) ? FACEUP : FACEDOWN);

                for (SLOT s = 0; s < MAXSLOT; s++)
                {
                    card *c2 = p[pos]->Card(s);
                    int x = c2->GetX() - c->GetX();
                    int y = c2->GetY() - c->GetY();
                    if (!c2->IsPlayed())
                        c2->Draw(memdc, x, y, mode, false);
                }
            }

            // Everyone needs to check for overlap of played cards.

            for (int j = playerled; j < i; j++)
            {
                card *c2 = handinfo.cardplayed[j % 4];
                int x = c2->GetX() - c->GetX();
                int y = c2->GetY() - c->GetY();
                c2->Draw(memdc, x, y, FACEUP, false);
            }

            memdc.SelectObject(wxNullBitmap);
        }
        card::m_bmBgnd = bgBmp;

        card *c = handinfo.cardplayed[i % 4];
        p[poswinner]->WinCard(dc, c);
        c->Remove();
    }

    ResetHandInfo(trickwinner);

    // If there are more tricks left before we need to reshuffle,
    // ask the winner of this trick to start next hand, and we're done.

    if (--tricksleft)
    {
        p[poswinner]->SelectCardToPlay(handinfo, bCheating);

        if (poswinner != 0)
            ((local_human *)p[0])->WaitMessage(p[poswinner]->GetName());

        if (::cQdMoves > 0)
        {
            for (int i = 0; i < ::cQdMoves; i++)
                HandleMove(::moveq[i]);

            ::cQdMoves = 0;
        }

        return;
    }

    // Make sure sound buffer is freed up.

    HeartsPlaySound(OFF);

    // Display hearts (and queen of spades) next to whoever "won" them.

    int nMoonShot = EMPTY;                  // assume nobody shot moon
    for (int i = 0; i < MAXPLAYER; i++)
    {
        bool bMoonShot;
        score[i] = p[i]->EvaluateScore(bMoonShot);
        if (bMoonShot)
            nMoonShot = i;                  // scores need to be adjusted

        wxClientDC dc(this);
        p[i]->DisplayHeartsWon(dc);
        p[i]->SetMode(SCORING);
    }

    // adjust scores if someone collected all hearts AND queen of spades

    if (nMoonShot != EMPTY)
    {
        for (int i = 0; i < MAXPLAYER; i++)
        {
            if (i == nMoonShot)
                score[i] -= 26;
            else
                score[i] += 26;

            p[i]->SetScore(score[i]);       // adjust player score manually
        }
    }

    // Show score

    p[0]->UpdateStatus(IDS_SCORE);
    p[0]->SetMode(SCORING);

    CScoreDlg scoredlg(this, score, m_myid);    // update scores in scoredlg

    player *pold = p[0];

    scoredlg.ShowModal();                        // display scores

    // If there has been a request to shut down while the score dialog
    // is displayed, m_FatalErrno will be non-zero.

    if (m_FatalErrno != 0)
    {
        p[0]->SetMode(PLAYING);         // something other than SCORING...
        FatalError(m_FatalErrno);       // so FatalError will accept it.
        return;
    }

    // It's possible for another player to have quit the game while
    // the score dialog was showing, so check that we're still
    // alive and well.

    if (p[0] != pold)
        return;

    // replace quit remote humans with computer players

    for (int i = 1; i < MAXPLAYER; i++)
    {
        if (p[i]->HasQuit())
        {
            wxString name = p[i]->GetName();
            int scoreLocal = p[i]->GetScore();
            delete p[i];
            p[i] = new computer(i);             // check for failure
            wxClientDC dc(this);
            p[i]->SetName(name, dc);
            p[i]->SetScore(scoreLocal);
        }
    }

    p[0]->SetMode(passdir == NOPASS ? DONE_SELECTING : SELECTING);

    if (scoredlg.IsGameOver())
    {
        GameOver();
        return;
    }

    Shuffle();

    // If there is no passing for upcoming round, we must make the changes
    // that HandlePassing() would normally do to start the next round.

    if (passdir == NOPASS)
    {
        for (int i = 0; i < MAXPLAYER; i++)         // everyone's DONE_SELECTING
            p[i]->SetMode(DONE_SELECTING);

        passdir = LEFT_DIR;                     // NEXT hand passes left
        tricksleft = MAXSLOT;                   // reset # of hands
        FirstMove();                            // start next trick
    }

    for (int i = 0; i < ::cQdMoves; i++)
        HandleMove(::moveq[i]);

    ::cQdMoves = 0;

    for (int i = 0; i < ::cQdPasses; i++)
        HandlePass(::passq[i]);

    ::cQdPasses = 0;
}


/****************************************************************************

CMainWindow::ResetHandInfo

Note that handinfo.bHeartsBroken is not reset here -- it applies to
the entire hand and is set only in FirstMove()

Same with handinfo.bQSPlayed and moonshoot variables.

****************************************************************************/

void CMainWindow::ResetHandInfo(int playernumber)
{
    handinfo.playerled = playernumber;
    handinfo.turn      = playernumber;
    for (int i = 0; i < MAXPLAYER; i++)
        handinfo.cardplayed[i] = nullptr;
}


/****************************************************************************

CMainWindow::HandleMove

Process a move from a player (local or queued).

****************************************************************************/

void CMainWindow::HandleMove(MOVE &move)
{
    int id = move.playerid;
    int pos = Id2Pos(id);
    int cardid = move.cardid;

    SLOT s = p[pos]->GetSlot(cardid);
    p[pos]->MarkCardPlayed(s);
    handinfo.cardplayed[id] = p[pos]->Card(s);
    handinfo.playerled = move.playerled;
    handinfo.turn = move.turn;

    OnRef();
}


/****************************************************************************

CMainWindow::HandlePass

Process a pass from a player (queued).

****************************************************************************/

void CMainWindow::HandlePass(PASS3 &pass3)
{
    int id = pass3.id;
    int pos = Id2Pos(id);

    for (int i = 0; i < 3; i++)
    {
        SLOT s = p[pos]->GetSlot(pass3.cardid[i]);
        p[pos]->Select(s, true);
    }

    p[pos]->SetMode(DONE_SELECTING);

    // Check if everyone is ready

    bool bReady = true;
    for (int i = 0; i < MAXPLAYER; i++)
        if (p[i]->GetMode() != DONE_SELECTING)
            bReady = false;

    if (bReady)
        HandlePassing();
}


