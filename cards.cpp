/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

cards.cpp -- wxWidgets port of cards.c

Card bitmap loading and drawing using wxBitmap.
Loads BMP files from compiled XRC resources (hearts.xrc).

****************************************************************************/

#include "card.h"
#include <wx/xrc/xmlres.h>
#include <wx/image.h>

/* Card bitmap filenames indexed by card ID.
   Card IDs: 0-12 = A-K of clubs, etc.
   id = value * 4 + suit  =>  suit = id % 4, value = id / 4
   But bitmaps are organized as suit*13 + rank in the resource file.
   Filenames: CLA, CL2..CL10, CLJ, CLQ, CLK, DMA..DMK, HTA..HTK, SPA..SPK
*/

static const wxChar* s_suitPrefix[4] = {
    wxT("cl"),   /* clubs */
    wxT("dm"),   /* diamonds */
    wxT("ht"),   /* hearts */
    wxT("sp")    /* spades */
};

static const wxChar* s_rankSuffix[13] = {
    wxT("a"), wxT("2"), wxT("3"), wxT("4"), wxT("5"),
    wxT("6"), wxT("7"), wxT("8"), wxT("9"), wxT("10"),
    wxT("j"), wxT("q"), wxT("k")
};

bool card::LoadCardBitmaps()
{
    if (m_bitmapsLoaded)
        return true;

    /* Load the 52 card face bitmaps from XRC resources.
       Card ID mapping: id = value * 4 + suit
       value: 0=Ace, 1=2, 2=3, ... 10=Jack, 11=Queen, 12=King
       suit:  0=clubs, 1=diamonds, 2=hearts, 3=spades
    */
    for (int id = 0; id < 52; id++) {
        int suit  = id % 4;
        int value = id / 4;
        wxString resName = wxString(wxT("card_")) + s_suitPrefix[suit] + s_rankSuffix[value];

        m_bmCard[id] = wxXmlResource::Get()->LoadBitmap(resName);
        if (!m_bmCard[id].IsOk()) {
            wxLogError(wxT("Failed to load card bitmap from XRC: %s"), resName);
            return false;
        }
    }

    /* Get card dimensions from first loaded bitmap */
    if (m_bmCard[0].IsOk()) {
        dxCrd = m_bmCard[0].GetWidth();
        dyCrd = m_bmCard[0].GetHeight();
    }

    /* Load card back bitmap from XRC */
    m_bmBack = wxXmlResource::Get()->LoadBitmap(wxT("card_back"));

    /* If no back, try the alternate pattern back */
    if (!m_bmBack.IsOk())
        m_bmBack = wxXmlResource::Get()->LoadBitmap(wxT("card_rbhatch"));

    /* Load ghost bitmap from XRC */
    m_bmGhost = wxXmlResource::Get()->LoadBitmap(wxT("card_ghost"));

    m_bitmapsLoaded = true;
    return true;
}

bool card::DrawCard(wxDC &dc, int x, int y, int dx, int dy,
                    int cd, int mode)
{
    if (!m_bitmapsLoaded)
        LoadCardBitmaps();

    wxBitmap *srcBmp = nullptr;

    switch (mode) {
    case FACEUP:
        if (cd >= 0 && cd < 52)
            srcBmp = &m_bmCard[cd];
        break;
    case FACEDOWN:
        srcBmp = &m_bmBack;
        break;
    case HILITE:
        if (cd >= 0 && cd < 52)
            srcBmp = &m_bmCard[cd];
        break;
    default:
        return false;
    }

    if (!srcBmp || !srcBmp->IsOk())
        return false;

    wxMemoryDC memDC;
    memDC.SelectObject(*srcBmp);

    if (dx != dxCrd || dy != dyCrd) {
        /* Scale */
        dc.StretchBlit(x, y, dx, dy, &memDC, 0, 0, dxCrd, dyCrd);
    } else {
        if (mode == HILITE) {
            /* Draw inverted */
            dc.Blit(x, y, dxCrd, dyCrd, &memDC, 0, 0, wxINVERT);
        } else {
            dc.Blit(x, y, dxCrd, dyCrd, &memDC, 0, 0);
        }
    }

    memDC.SelectObject(wxNullBitmap);

    /* Draw border for red cards (hearts, diamonds) in FACEUP mode */
    if (mode == FACEUP && cd >= 0 && cd < 52) {
        int suit = cd % 4;
        if (suit == DIAMONDS || suit == HEARTS) {
            dc.SetPen(*wxBLACK_PEN);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            /* Top */
            dc.DrawLine(x+2, y, x+dx-2, y);
            /* Right */
            dc.DrawLine(x+dx-1, y+2, x+dx-1, y+dy-2);
            /* Bottom */
            dc.DrawLine(x+2, y+dy-1, x+dx-2, y+dy-1);
            /* Left */
            dc.DrawLine(x, y+2, x, y+dy-2);
            /* Corner pixels */
            dc.DrawPoint(x+1, y+1);
            dc.DrawPoint(x+dx-2, y+1);
            dc.DrawPoint(x+dx-2, y+dy-2);
            dc.DrawPoint(x+1, y+dy-2);
        }
    }

    return true;
}
