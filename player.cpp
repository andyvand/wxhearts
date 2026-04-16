/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

player.cpp -- wxWidgets port

Methods for player objects

****************************************************************************/

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include "debug.h"

#include <cstdlib>

extern "C" {
int CompareCards(const void *v1, const void *v2);
}


player::player(int n, int pos) : id(n), position(pos)
{
    /* Set up font */
    wxString fontname = GetStringResource(IDS_FONTFACE);
    long fontsize = 0;
    GetStringResource(IDS_FONTSIZE).ToLong(&fontsize);
    if (fontsize <= 0) fontsize = 10;
    font = wxFont(wxFontInfo(fontsize).FaceName(fontname).Bold());

    wxRect rect = CMainWindow::m_TableRect;

    wxPoint centre;
    const int offset = 30;

    mode = STARTING;

    centre.x = (rect.GetRight() / 2) - (card::dxCrd / 2);
    centre.y = (rect.GetBottom() / 2) - (card::dyCrd / 2);
    playloc = centre;
    score = 0;

    /* Get text height */
    int nTextHeight;
    {
        wxClientDC dc(pMainWnd);
        wxCoord w, h;
        dc.GetTextExtent(wxT("Xg"), &w, &h);
        nTextHeight = h;
    }

    switch (position) {
        case 0:
            loc.x = (rect.GetRight() - (12 * HORZSPACING + card::dxCrd)) / 2;
            loc.y = rect.GetBottom() - card::dyCrd - IDGE;
            dx = HORZSPACING;
            dy = 0;
            playloc.x -= 5;
            playloc.y += offset;
            dotloc.x = loc.x + (HORZSPACING / 2);
            dotloc.y = loc.y - IDGE;
            homeloc.x = playloc.x;
            homeloc.y = rect.GetBottom() + card::dyCrd;
            nameloc.x = loc.x + card::dxCrd + IDGE;
            nameloc.y = rect.GetBottom() - nTextHeight - IDGE;
            break;

        case 1:
            loc.x = 3 * IDGE;
            loc.y = ((rect.GetBottom() - (12 * VERTSPACING + card::dyCrd)) / 2);
            dx = 0;
            dy = VERTSPACING;
            playloc.x -= offset;
            playloc.y -= 5;
            dotloc.x = loc.x + card::dxCrd + IDGE;
            dotloc.y = loc.y + (VERTSPACING / 2);
            homeloc.x = -card::dxCrd;
            homeloc.y = playloc.y;
            nameloc.x = loc.x + 2;
            nameloc.y = loc.y - nTextHeight;
            break;

        case 2:
            loc.x = ((rect.GetRight() - (12 * HORZSPACING + card::dxCrd)) / 2)
                    + (12 * HORZSPACING);
            loc.y = IDGE + MENU_HEIGHT;
            dx = -HORZSPACING;
            dy = 0;
            playloc.x += 5;
            playloc.y -= offset;
            dotloc.x = loc.x + card::dxCrd - (HORZSPACING / 2);
            dotloc.y = loc.y + card::dyCrd + IDGE;
            homeloc.x = playloc.x;
            homeloc.y = -card::dyCrd;
            nameloc.x = ((rect.GetRight() - (12 * HORZSPACING + card::dxCrd)) / 2)
                        + (12 * HORZSPACING) + card::dxCrd + IDGE;
            nameloc.y = IDGE + MENU_HEIGHT;
            break;

        case 3:
            loc.x = rect.GetRight() - (card::dxCrd + (3 * IDGE));
            loc.y = ((rect.GetBottom() - (12 * VERTSPACING + card::dyCrd)) / 2)
                   + (12 * VERTSPACING);
            dx = 0;
            dy = -VERTSPACING;
            playloc.x += offset;
            playloc.y += 5;
            dotloc.x = loc.x - IDGE;
            dotloc.y = loc.y + card::dyCrd - (VERTSPACING / 2);
            homeloc.x = rect.GetRight();
            homeloc.y = playloc.y;
            nameloc.x = ((rect.GetRight() - (12 * HORZSPACING + card::dxCrd)) / 2)
                         - IDGE - 2;
            nameloc.y = ((rect.GetBottom() - (12 * VERTSPACING + card::dyCrd)) / 2)
                   + (12 * VERTSPACING) + card::dyCrd;
            break;
    }

    ResetLoc();
}


void player::ResetLoc()
{
    int x = loc.x;
    int y = loc.y;

    for (SLOT s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsInHand())
            cd[s].SetLoc(x, y);
        x += dx;
        y += dy;
    }
}


void player::Sort()
{
    qsort((void *)cd, MAXSLOT, sizeof(card),
          CompareCards);
    ResetLoc();
}


int CompareCards(const void *v1, const void *v2)
{
    card *c1 = (card *)v1;
    card *c2 = (card *)v2;

    int val1 = c1->Value2();
    int val2 = c2->Value2();
    int s1 = c1->Suit();
    int s2 = c2->Suit();

    if (!(c1->IsInHand()))
        val1 = EMPTY;
    if (!(c2->IsInHand()))
        val2 = EMPTY;

    if (val1 == EMPTY || val2 == EMPTY)
    {
        if (val1 == val2) return 0;
        else if (val1 == EMPTY) return 1;
        else return -1;
    }

    if (s1 != s2)
    {
        if (s1 == HEARTS && s2 == SPADES) return 1;
        else if (s1 == SPADES && s2 == HEARTS) return -1;
        else return (s1 - s2);
    }

    return (val1 - val2);
}


SLOT player::GetSlot(int id)
{
    SLOT s = EMPTY;
    for (int num = 0; num < MAXSLOT; num++)
    {
        if (GetID(num) == id) { s = num; break; }
    }
    wxASSERT(s != EMPTY);
    return s;
}


bool player::GetCardLoc(SLOT s, wxPoint &loc)
{
    if (!cd[s].IsValid())
        return false;

    loc.x = cd[s].GetX();
    loc.y = cd[s].GetY();
    return true;
}


wxRect &player::GetCoverRect(wxRect &rect)
{
    int left  = (dx < 0 ? loc.x + 12 * dx : loc.x);
    int right = left + (dx != 0 ? card::dxCrd + 12 * abs(dx) : card::dxCrd);
    int top   = (dy < 0 ? loc.y + 12 * dy : loc.y);
    int bottom = top + (dy != 0 ? card::dyCrd + 12 * abs(dy) : card::dyCrd);

    if (position == 0)      top -= POPSPACING;
    else if (position == 1) right += 2 * IDGE;
    else if (position == 2) bottom += 2 * IDGE;
    else                    left -= 2 * IDGE;

    rect = wxRect(left, top, right - left, bottom - top);
    return rect;
}


wxRect &player::GetMarkingRect(wxRect &rect)
{
    int left   = (dx < 0 ? dotloc.x + (12 * dx) : dotloc.x);
    int right  = (dx < 0 ? dotloc.x + 2 : dotloc.x + (12 * dx) + 2);
    int top    = (dy < 0 ? dotloc.y + (12 * dy) : dotloc.y);
    int bottom = (dy < 0 ? dotloc.y + 2 : dotloc.y + (12 * dy) + 2);

    rect = wxRect(left, top, right - left, bottom - top);
    return rect;
}


void player::Draw(wxDC &dc, bool bCheating, SLOT slot)
{
    DisplayName(dc);
    SLOT start = (slot == ALL ? 0 : slot);
    SLOT stop  = (slot == ALL ? MAXSLOT : slot + 1);

    SLOT playedslot = EMPTY;

    for (SLOT s = start; s < stop; s++)
    {
        if (cd[s].IsPlayed())
            playedslot = s;
        else if (bCheating)
            cd[s].Draw(dc);
        else
            cd[s].Draw(dc, FACEDOWN);
    }

    if (playedslot != EMPTY)
        cd[playedslot].Draw(dc);
}

void player::DisplayName(wxDC &dc, const wxRect *cardBounds)
{
    wxFont oldFont = dc.GetFont();
    dc.SetFont(font);
    dc.SetTextBackground(pMainWnd->GetBkColor());
    dc.SetBackgroundMode(wxTRANSPARENT);

    wxCoord w, h;
    dc.GetTextExtent(name, &w, &h);

    wxRect bounds;
    bool haveBounds = false;

    if (cardBounds) {
        bounds = *cardBounds;
        haveBounds = true;
    } else {
        // Compute bounding box from cards currently in hand
        int minX = 99999, maxX = -99999;
        int minY = 99999, maxY = -99999;

        for (SLOT s = 0; s < MAXSLOT; s++) {
            if (cd[s].IsInHand()) {
                int cx = cd[s].GetX();
                int cy = cd[s].GetY();
                if (cx < minX) minX = cx;
                if (cx + card::dxCrd > maxX) maxX = cx + card::dxCrd;
                if (cy < minY) minY = cy;
                if (cy + card::dyCrd > maxY) maxY = cy + card::dyCrd;
                haveBounds = true;
            }
        }

        if (haveBounds)
            bounds = wxRect(minX, minY, maxX - minX, maxY - minY);
    }

    wxPoint namepos = nameloc; // fallback when no cards visible

    if (haveBounds) {
        int left   = bounds.GetX();
        int top    = bounds.GetY();
        int right  = bounds.GetX() + bounds.GetWidth();
        int bottom = bounds.GetY() + bounds.GetHeight();

        switch (position) {
            case 0: // bottom - name to the left of cards, bottom-aligned
                namepos.x = left - IDGE - w;
                namepos.y = bottom - h;
                break;
            case 1: // left - name above cards, left-aligned
                namepos.x = left;
                namepos.y = top - h;
                break;
            case 2: // top - name to the right of cards, top-aligned
                namepos.x = right + IDGE;
                namepos.y = top;
                break;
            case 3: // right - name below cards, right-aligned
                namepos.x = right - w;
                namepos.y = bottom + IDGE;
                break;
        }
    }

    dc.DrawText(name, namepos.x, namepos.y);
    dc.SetFont(oldFont);
}

void player::SetName(wxString &newname, wxDC &dc)
{
    name = newname;
}


void player::ReturnSelectedCards(int c[])
{
    c[0] = EMPTY;
    c[1] = EMPTY;
    c[2] = EMPTY;

    if (mode == STARTING || mode == SELECTING)
        return;

    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
            c[j++] = cd[i].ID();
        if (i >= MAXSLOT) { wxASSERT(i < MAXSLOT); }
    }
}

void player::ReceiveSelectedCards(int c[])
{
    for (int i = 0, j = 0; j < 3; i++)
    {
        if (cd[i].IsSelected())
        {
            cd[i].SetID(c[j++]);
            cd[i].Select(false);
        }
        wxASSERT(i < MAXSLOT);
    }
    SetMode(WAITING);
}


void player::MarkSelectedCards(wxDC &dc)
{
    wxColour color(255, 255, 255);

    for (int s = 0; s < MAXSLOT; s++)
    {
        if (cd[s].IsSelected())
        {
            int x = dotloc.x + (s * dx);
            int y = dotloc.y + (s * dy);
            dc.SetPen(wxPen(color));
            dc.DrawPoint(x, y);
            dc.DrawPoint(x+1, y);
            dc.DrawPoint(x, y+1);
            dc.DrawPoint(x+1, y+1);
        }
    }
}


void player::GlideToCentre(SLOT s, bool bFaceup)
{
    wxRect rectCard, rectSrc;

    wxClientDC dc(pMainWnd);

    wxBitmap bgBmp(card::dxCrd, card::dyCrd);
    {
        wxMemoryDC memdc;
        memdc.SelectObject(bgBmp);
        memdc.SetBrush(CMainWindow::m_BgndBrush);
        memdc.SetPen(*wxTRANSPARENT_PEN);
        memdc.DrawRectangle(0, 0, card::dxCrd, card::dyCrd);

        cd[s].GetRect(rectCard);

        for (SLOT i = 0; i < MAXSLOT; i++)
        {
            if (cd[i].IsNormal() && (i != s))
            {
                cd[i].GetRect(rectSrc);
                if (rectSrc.Intersects(rectCard))
                {
                    cd[i].Draw(memdc,
                               rectSrc.GetLeft() - rectCard.GetLeft(),
                               rectSrc.GetTop() - rectCard.GetTop(),
                               bFaceup ? FACEUP : FACEDOWN,
                               false);
                }
            }
        }
        memdc.SelectObject(wxNullBitmap);
    }
    card::m_bmBgnd = bgBmp;

    cd[s].CleanDraw(dc);
    cd[s].Glide(dc, playloc.x, playloc.y);
    cd[s].Play();

    SetMode(WAITING);
}


void player::ResetCardsWon()
{
    for (int i = 0; i < MAXCARDSWON; i++)
        cardswon[i] = EMPTY;
    numcardswon = 0;
}


void player::WinCard(wxDC &dc, card *c)
{
    if ((c->IsHeart()) || (c->ID() == BLACKLADY))
        cardswon[numcardswon++] = c->ID();

    RegEntry Reg(szRegPath);
    long dwSpeed = Reg.GetNumber(wxT("speed"), IDC_NORMAL);

    int oldstep = c->SetStepSize(dwSpeed == IDC_SLOW ? 5 : 30);
    c->Glide(dc, homeloc.x, homeloc.y);
    c->SetStepSize(oldstep);
}


int player::EvaluateScore(bool &bMoonShot)
{
    for (int i = 0; i < MAXCARDSWON; i++)
    {
        if (cardswon[i] == BLACKLADY)
            score += 13;
        else if (cardswon[i] != EMPTY)
            score++;
    }

    bMoonShot = (cardswon[MAXCARDSWON-1] != EMPTY);
    return score;
}


void player::DisplayHeartsWon(wxDC &dc)
{
    card c;
    int x = loc.x;
    int y = loc.y;

    x += ((MAXCARDSWON - numcardswon) / 2) * dx;
    y += ((MAXCARDSWON - numcardswon) / 2) * dy;

    int startX = x, startY = y;

    for (int i = 0; i < numcardswon; i++)
    {
        c.SetID(cardswon[i]);
        c.SetLoc(x, y);
        c.Draw(dc);
        x += dx;
        y += dy;
    }

    if (numcardswon > 0) {
        int endX = x - dx;
        int endY = y - dy;
        int minX = (startX < endX) ? startX : endX;
        int maxX = ((startX > endX) ? startX : endX) + card::dxCrd;
        int minY = (startY < endY) ? startY : endY;
        int maxY = ((startY > endY) ? startY : endY) + card::dyCrd;
        wxRect bounds(minX, minY, maxX - minX, maxY - minY);
        DisplayName(dc, &bounds);
    } else {
        DisplayName(dc);
    }
}
