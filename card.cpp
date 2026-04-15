/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

card.cpp -- wxWidgets port

Methods for card objects.  Card drawing uses wxBitmap loaded from bmp/.

****************************************************************************/

#include "card.h"
#include "main.h"
#include <cstdlib>

/* Declare (and initialize) static members */

wxBitmap card::m_bmFgnd;
wxBitmap card::m_bmBgnd2;
wxBitmap card::m_bmCard[52];
wxBitmap card::m_bmBack;
wxBitmap card::m_bmGhost;
bool     card::m_bitmapsLoaded = false;

bool     card::bConstructed = false;
int      card::dxCrd = 71;
int      card::dyCrd = 96;
wxBitmap card::m_bmBgnd;

int      card::count    = 0;
int      card::stepsize = 15;


card::card(int n) : id(n), state(NORMAL)
{
    loc.x = 0;
    loc.y = 0;
    if (count == 0)
    {
        bConstructed = false;

        if (!LoadCardBitmaps())
            return;

        bConstructed = true;

        /* Create working bitmaps for animation */
        m_bmBgnd = wxBitmap(dxCrd, dyCrd);
        m_bmFgnd = wxBitmap(dxCrd, dyCrd);
        m_bmBgnd2 = wxBitmap(dxCrd, dyCrd);
    }
    count++;
}


card::~card()
{
    count--;
    if (count == 0)
    {
        m_bmBgnd = wxNullBitmap;
        m_bmFgnd = wxNullBitmap;
        m_bmBgnd2 = wxNullBitmap;
    }
}


bool card::Draw(wxDC &dc, int x, int y, int mode, bool bUpdateLoc)
{
    if (bUpdateLoc)
    {
        loc.x = x;
        loc.y = y;
    }

    if (id == EMPTY)
        return false;

    return DrawCard(dc, x, y, dxCrd, dyCrd,
        mode == FACEDOWN ? CARDBACK : id, mode);
}


bool card::CleanDraw(wxDC &dc)
{
    if (id == EMPTY)
        return false;

    wxBitmap bitmap(dxCrd, dyCrd);
    wxMemoryDC memDC;
    memDC.SelectObject(bitmap);

    DrawCard(memDC, 0, 0, dxCrd, dyCrd, id, FACEUP);

    dc.Blit(loc.x, loc.y, dxCrd, dyCrd, &memDC, 0, 0);
    memDC.SelectObject(wxNullBitmap);

    return true;
}


bool card::PopDraw(wxDC &dc)
{
    if (id == EMPTY)
        return false;

    int y = loc.y;
    if (state == SELECTED)
        y -= POPSPACING;

    return DrawCard(dc, loc.x, y, dxCrd, dyCrd, id, FACEUP);
}


void card::Glide(wxDC &dc, int xEnd, int yEnd)
{
    (void)dc;   // animation draws into CMainWindow::m_backbuffer; the
                // DC passed by callers (historically a wxClientDC on
                // the main window) is no longer used -- see GlideStep.

    /* Suppress scene re-render during animation so it doesn't overwrite
       the frames GlideStep is writing into the backbuffer. */
    if (pMainWnd)
        pMainWnd->bAnimating = true;

    /* Draw card face into fgnd bitmap, then release it */
    {
        wxMemoryDC memFg;
        memFg.SelectObject(m_bmFgnd);
        DrawCard(memFg, 0, 0, dxCrd, dyCrd, id, FACEUP);
        memFg.SelectObject(wxNullBitmap);
    }

    long dx = xEnd - loc.x;
    long dy = yEnd - loc.y;
    int  distance = IntSqrt(dx*dx + dy*dy);
    int  steps = distance / stepsize;

    if ((steps % 2) == 1)
        steps++;

    int x1 = loc.x;
    int y1 = loc.y;
    for (int i = 1; i < steps; i++)
    {
        int x2 = loc.x + (int)(((long)i * dx) / (long)steps);
        int y2 = loc.y + (int)(((long)i * dy) / (long)steps);
        GlideStep(x1, y1, x2, y2);
        x1 = x2;
        y1 = y2;
    }

    /* Last step lands exactly on target */
    GlideStep(x1, y1, xEnd, yEnd);

    loc.x = xEnd;
    loc.y = yEnd;

    /* Animation finished.  Clear bAnimating and force one more paint
       so OnPaint re-renders the scene from state (card now in its new
       logical position).  Without this, the backbuffer would keep the
       last animation frame until the next state-change-driven repaint,
       which may include stale overlap artifacts. */
    if (pMainWnd)
    {
        pMainWnd->bAnimating = false;
        pMainWnd->Refresh(false);
        pMainWnd->Update();
    }
}


void card::GlideStep(int x1, int y1, int x2, int y2)
{
    if (!pMainWnd || !pMainWnd->m_backbuffer.IsOk())
        return;

    /* Draw the animation frame directly into the main window's
       backbuffer.  OnPaint will blit the backbuffer to the screen,
       so this is the only path that reliably puts pixels on the
       window under wxGTK3 (where wxClientDC writes are not committed
       to the backing store that the compositor reads from). */
    {
        wxMemoryDC dc;
        dc.SelectObject(pMainWnd->m_backbuffer);

        wxMemoryDC memB, memB2;
        memB.SelectObject(m_bmBgnd);
        memB2.SelectObject(m_bmBgnd2);

        /* Create background of new location */
        memB2.Blit(0, 0, dxCrd, dyCrd, &dc, x2, y2);
        memB2.Blit(x1-x2, y1-y2, dxCrd, dyCrd, &memB, 0, 0);

        /* Draw old background where card was */
        dc.Blit(x1, y1, dxCrd, dyCrd, &memB, 0, 0);

        /* Draw card at new position */
        wxMemoryDC fgDC;
        fgDC.SelectObject(m_bmFgnd);
        dc.Blit(x2, y2, dxCrd, dyCrd, &fgDC, 0, 0);
        fgDC.SelectObject(wxNullBitmap);

        memB.SelectObject(wxNullBitmap);
        memB2.SelectObject(wxNullBitmap);
        dc.SelectObject(wxNullBitmap);
    }

    /* Flush the backbuffer to screen via OnPaint.  bAnimating is
       true, so OnPaint skips re-rendering the scene and just blits
       the backbuffer we just wrote to.  Refresh only the rectangle
       covering both old and new card positions to minimise work. */
    int xMin = (x1 < x2) ? x1 : x2;
    int yMin = (y1 < y2) ? y1 : y2;
    int xMax = ((x1 > x2) ? x1 : x2) + dxCrd;
    int yMax = ((y1 > y2) ? y1 : y2) + dyCrd;
    wxRect rect(xMin, yMin, xMax - xMin, yMax - yMin);
    pMainWnd->RefreshLogicalRect(rect, false);
    pMainWnd->Update();

    /* Swap backgrounds */
    wxBitmap temp = m_bmBgnd;
    m_bmBgnd = m_bmBgnd2;
    m_bmBgnd2 = temp;
}


int card::IntSqrt(long square)
{
    long lastguess = square;
    long guess = std::min(square / 2L, 1024L);

    if (guess == 0)
        return 0;

    while (labs(guess - lastguess) > 3L)
    {
        lastguess = guess;
        guess -= ((guess * guess) - square) / (2L * guess);
    }

    return (int)guess;
}


wxRect &card::GetRect(wxRect &rect)
{
    rect = wxRect(loc.x, loc.y, dxCrd, dyCrd);
    return rect;
}
