/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

dlg.cpp -- wxWidgets port

Aug 92, JimH
May 93, JimH    chico port

Dialog classes are defined here.

CScoreDlg       shows current score sheet
CQuoteDlg       quote dialog
CWelcomeDlg     welcome to Hearts, wanna be gamemeister?
COptionsDlg     set options

****************************************************************************/

#include "hearts.h"
#include "resource.h"
#include "main.h"
#include "debug.h"

#include <wx/statline.h>

/* nHandsPlayed is shared with C code */
extern "C" {
    int nHandsPlayed = 0;
}

/* Static member definitions */
int  CScoreDlg::score[MAXPLAYER][MAXHANDS + 1] = {};
bool CScoreDlg::bGameOver = false;

/*****************************************************************************

CScoreDlg

*****************************************************************************/

wxBEGIN_EVENT_TABLE(CScoreDlg, wxDialog)
    EVT_PAINT(CScoreDlg::OnPaint)
    EVT_BUTTON(wxID_OK, CScoreDlg::OnOK)
wxEND_EVENT_TABLE()

/****************************************************************************

CScoreDlg constructors

The first constructor takes only one argument, the pointer to the class
of the parent window.  It is used to display the current score at
arbitrary points in the game, ie when the user requests it.

The second also updates the static score array with new information.

****************************************************************************/

CScoreDlg::CScoreDlg(wxWindow *pParent)
    : wxDialog(pParent, wxID_ANY, wxEmptyString,
               wxDefaultPosition, wxSize(400, 250),
               wxDEFAULT_DIALOG_STYLE),
      m_myid(-1)
{
    SetText();

    /* Create OK button at bottom, centered */
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer(1);

    wxButton *okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();
    sizer->Add(okBtn, 0, wxALIGN_CENTER | wxALL, 5);
    SetSizer(sizer);

    CentreOnParent();
}

CScoreDlg::CScoreDlg(wxWindow *pParent, int s[MAXPLAYER], int id)
    : wxDialog(pParent, wxID_ANY, wxEmptyString,
               wxDefaultPosition, wxSize(400, 250),
               wxDEFAULT_DIALOG_STYLE),
      m_myid(id)
{
    if (nHandsPlayed < 0)
        nHandsPlayed = 0;

    /* If we have filled the score display, scroll up */
    if (nHandsPlayed == MAXHANDS)
    {
        for (int hand = 1; hand < MAXHANDS; hand++)
            for (int p = 0; p < MAXPLAYER; p++)
                score[p][hand - 1] = score[p][hand];

        nHandsPlayed--;
    }

    /* Add latest scores */
    for (int p = 0; p < MAXPLAYER; p++)
        score[p][nHandsPlayed] = s[p];

    nHandsPlayed++;

    SetText();

    /* Create OK button at bottom, centered */
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer(1);

    wxButton *okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();
    sizer->Add(okBtn, 0, wxALIGN_CENTER | wxALL, 5);
    SetSizer(sizer);

    CentreOnParent();
}


/****************************************************************************

CScoreDlg::SetText  -- set title bar text

****************************************************************************/

void CScoreDlg::SetText()
{
    wxString s = GetStringResource(IDS_SCORESHEET);

    if (nHandsPlayed != 0)
    {
        int place = 0;
        for (int i = 1; i < MAXPLAYER; i++)
            if (score[i][nHandsPlayed - 1] < score[0][nHandsPlayed - 1])
                place++;

        wxString s2 = GetStringResource(IDS_PLACE1 + place);
        s += wxT(" -- ");
        s += s2;
    }

    SetTitle(s);
}


/****************************************************************************

CScoreDlg::OnPaint

The score text is not drawn with text controls because the strikeout
text is needed for some parts of the score.  Instead, the paint message
is hooked here.

****************************************************************************/

void CScoreDlg::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);

    wxString fontname = GetStringResource(IDS_FONTFACE);
    wxString charsetstr = GetStringResource(IDS_CHARSET);
    wxString fontsizestr = GetStringResource(IDS_FONTSIZE);
    int fontsize = wxAtoi(fontsizestr);
    if (fontsize == 0) fontsize = 13;

    /* Nobody has best score if game hasn't started yet */
    int nBestScore = (nHandsPlayed == 0 ? 0 : 30000);
    int nWorstScore = 0;

    if (nHandsPlayed > 0)
    {
        for (int pos = 0; pos < MAXPLAYER; pos++)
        {
            if (score[pos][nHandsPlayed - 1] < nBestScore)
                nBestScore = score[pos][nHandsPlayed - 1];
            if (score[pos][nHandsPlayed - 1] > nWorstScore)
                nWorstScore = score[pos][nHandsPlayed - 1];
        }
    }

    /* If the game is over, display appropriate text in title bar */
    if (nWorstScore >= 100)
    {
        wxString title;
        if (score[0][nHandsPlayed - 1] == nBestScore)
            title = GetStringResource(IDS_GAMEOVERWIN);
        else
            title = GetStringResource(IDS_GAMEOVER);

        SetTitle(title);
        bGameOver = true;
    }

    wxRect rect = GetClientRect();

    /* Divide the dialog up into columns for displaying scores */
    rect.width -= 5;
    int nWidth = rect.width / 5;
    int nHeight = rect.height - 40; /* leave room for OK button */
    dc.SetBackgroundMode(wxTRANSPARENT);

    /* Create bold font and bold strikeout font */
    wxFont boldFont(wxFontInfo(fontsize).FaceName(fontname).Bold());
    wxFont strikeFont(wxFontInfo(fontsize).FaceName(fontname).Bold().Strikethrough());

    for (int pos = 0; pos < MAXPLAYER; pos++)
    {
        /* Set text colour for winner */
        if (nHandsPlayed > 0)
        {
            if (score[pos][nHandsPlayed - 1] == nBestScore)
                dc.SetTextForeground(bGameOver ? wxColour(127, 0, 0) : wxColour(0, 0, 255));
        }

        wxString text = pMainWnd->GetPlayerName(pos);

        /* Draw player name in bold */
        dc.SetFont(boldFont);
        int colLeft = (nWidth * pos);
        int colRight = colLeft + nWidth + 15;
        int colCenter = colLeft + (colRight - colLeft) / 2;

        wxCoord tw, th;
        dc.GetTextExtent(text, &tw, &th);
        dc.DrawText(text, colCenter - tw / 2, 5);
        int yPos = 5 + th + 2;

        /* Draw old scores with strikeout font */
        dc.SetFont(strikeFont);
        for (int hand = 0; hand < (nHandsPlayed - 1); hand++)
        {
            wxString scoreStr = wxString::Format(wxT("%d"), score[pos][hand]);
            dc.GetTextExtent(scoreStr, &tw, &th);
            dc.DrawText(scoreStr, colCenter - tw / 2, yPos);
            yPos += th;
        }

        /* Draw current score in bold (not strikeout) */
        dc.SetFont(boldFont);
        if (nHandsPlayed > 0)
        {
            wxString scoreStr = wxString::Format(wxT("%d"), score[pos][nHandsPlayed - 1]);
            dc.GetTextExtent(scoreStr, &tw, &th);
            dc.DrawText(scoreStr, colCenter - tw / 2, yPos);
        }

        dc.SetTextForeground(*wxBLACK);
    }
}


/****************************************************************************

CScoreDlg::OnOK

****************************************************************************/

void CScoreDlg::OnOK(wxCommandEvent &event)
{
    EndModal(wxID_OK);
}


/*****************************************************************************

CQuoteDlg

*****************************************************************************/

wxBEGIN_EVENT_TABLE(CQuoteDlg, wxDialog)
    EVT_PAINT(CQuoteDlg::OnPaint)
    EVT_BUTTON(wxID_OK, CQuoteDlg::OnOK)
wxEND_EVENT_TABLE()


/****************************************************************************

CQuoteDlg constructor

****************************************************************************/

CQuoteDlg::CQuoteDlg(wxWindow *pParent)
    : wxDialog(pParent, wxID_ANY,
               wxT("Quote for The Microsoft Hearts Network"),
               wxDefaultPosition, wxSize(380, 180),
               wxDEFAULT_DIALOG_STYLE)
{
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddSpacer(50); /* Leave room for icon painted in OnPaint */

    wxStaticText *quote = new wxStaticText(this, wxID_ANY,
        wxT("I come not, friends, to steal away your hearts..."),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(quote, 0, wxALL | wxEXPAND, 10);

    wxStaticText *cite = new wxStaticText(this, wxID_ANY,
        wxT("- Julius Caesar, Act III, scene ii"),
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(cite, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);

    sizer->AddStretchSpacer(1);

    wxButton *okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();
    sizer->Add(okBtn, 0, wxALIGN_CENTER | wxALL, 10);

    SetSizer(sizer);
    CentreOnParent();
}


/****************************************************************************

CQuoteDlg::OnPaint

Draws the application icon in the upper-left area.

****************************************************************************/

void CQuoteDlg::OnPaint(wxPaintEvent &event)
{
    wxPaintDC dc(this);

    /* Draw a small heart symbol in the corner as a stand-in for the icon */
    dc.SetTextForeground(wxColour(200, 0, 0));
    wxFont iconFont(wxFontInfo(24).Bold());
    dc.SetFont(iconFont);
    dc.DrawText(wxT("\xe2\x99\xa5"), 24, 10); /* Unicode heart U+2665 */
}


/****************************************************************************

CQuoteDlg::OnOK

****************************************************************************/

void CQuoteDlg::OnOK(wxCommandEvent &event)
{
    EndModal(wxID_OK);
}


/*****************************************************************************

CWelcomeDlg

*****************************************************************************/

wxBEGIN_EVENT_TABLE(CWelcomeDlg, wxDialog)
    EVT_BUTTON(wxID_OK, CWelcomeDlg::OnOK)
    EVT_BUTTON(wxID_CANCEL, CWelcomeDlg::OnCancel)
wxEND_EVENT_TABLE()


/****************************************************************************

CWelcomeDlg constructor

****************************************************************************/

CWelcomeDlg::CWelcomeDlg(wxWindow *pParent)
    : wxDialog(pParent, wxID_ANY,
               GetStringResource(IDS_APPNAME),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
    /* Read default name from config */
    RegEntry Reg(szRegPath);
    m_myname = Reg.GetString(regvalName);

    /* Build the dialog programmatically */
    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText *introText = new wxStaticText(this, wxID_ANY,
        GetStringResource(IDS_INTRO));
    topSizer->Add(introText, 0, wxALL, 10);

    wxBoxSizer *nameSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *nameLabel = new wxStaticText(this, wxID_ANY,
        wxT("What is your name?"));
    nameSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);

    wxTextCtrl *nameCtrl = new wxTextCtrl(this, IDC_YOURNAME, m_myname,
        wxDefaultPosition, wxSize(150, -1));
    nameCtrl->SetMaxLength(MAXNAMELENGTH);
    nameSizer->Add(nameCtrl, 1, wxEXPAND);

    topSizer->Add(nameSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

    /* OK and Quit buttons */
    wxStdDialogButtonSizer *btnSizer = new wxStdDialogButtonSizer();
    wxButton *okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();
    btnSizer->SetAffirmativeButton(okBtn);

    wxButton *cancelBtn = new wxButton(this, wxID_CANCEL, wxT("Quit"));
    btnSizer->SetCancelButton(cancelBtn);
    btnSizer->Realize();

    topSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizerAndFit(topSizer);
    CentreOnParent();

    nameCtrl->SetFocus();
}


/****************************************************************************

CWelcomeDlg::OnOK()

Don't allow empty name.  Store data in config.

****************************************************************************/

void CWelcomeDlg::OnOK(wxCommandEvent &event)
{
    wxTextCtrl *nameCtrl = wxDynamicCast(FindWindow(IDC_YOURNAME), wxTextCtrl);
    if (nameCtrl)
        m_myname = nameCtrl->GetValue();

    if (m_myname.IsEmpty())
    {
        if (nameCtrl)
            nameCtrl->SetFocus();
        return;
    }

    RegEntry Reg(szRegPath);
    Reg.SetValue(regvalRole, (long)1);
    Reg.SetValue(regvalName, m_myname);

    EndModal(wxID_OK);
}


/****************************************************************************

CWelcomeDlg::OnCancel()

****************************************************************************/

void CWelcomeDlg::OnCancel(wxCommandEvent &event)
{
    EndModal(wxID_CANCEL);
}


/*****************************************************************************

COptionsDlg

*****************************************************************************/

wxBEGIN_EVENT_TABLE(COptionsDlg, wxDialog)
    EVT_BUTTON(wxID_OK, COptionsDlg::OnOK)
wxEND_EVENT_TABLE()


/****************************************************************************

COptionsDlg constructor

Set dialog controls to current values.

****************************************************************************/

COptionsDlg::COptionsDlg(wxWindow *pParent)
    : wxDialog(pParent, wxID_ANY,
               wxT("Hearts Options"),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE)
{
    RegEntry Reg(szRegPath);

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    /* Animation speed radio buttons */
    wxStaticBoxSizer *speedBox = new wxStaticBoxSizer(wxVERTICAL, this,
        wxT("Animation speed"));

    wxRadioButton *slowBtn = new wxRadioButton(speedBox->GetStaticBox(),
        IDC_SLOW, wxT("&Slow"), wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    wxRadioButton *normalBtn = new wxRadioButton(speedBox->GetStaticBox(),
        IDC_NORMAL, wxT("&Normal"));
    wxRadioButton *fastBtn = new wxRadioButton(speedBox->GetStaticBox(),
        IDC_FAST, wxT("&Fast"));

    speedBox->Add(slowBtn, 0, wxALL, 3);
    speedBox->Add(normalBtn, 0, wxALL, 3);
    speedBox->Add(fastBtn, 0, wxALL, 3);

    /* Set current speed from config */
    long dwSpeed = Reg.GetNumber(regvalSpeed, IDC_NORMAL);
    if (dwSpeed == IDC_SLOW)
        slowBtn->SetValue(true);
    else if (dwSpeed == IDC_FAST)
        fastBtn->SetValue(true);
    else
        normalBtn->SetValue(true);

    topSizer->Add(speedBox, 0, wxALL | wxEXPAND, 10);

    /* Computer player name edit fields */
    wxStaticBoxSizer *nameBox = new wxStaticBoxSizer(wxVERTICAL, this,
        wxT("Computer player names"));

    for (int i = 0; i < 3; i++)
    {
        wxString sName = Reg.GetString(regvalPName[i]);
        if (sName.IsEmpty())
            sName = GetStringResource(IDS_P1NAME + i);

        wxTextCtrl *nameCtrl = new wxTextCtrl(nameBox->GetStaticBox(),
            IDC_NAME1 + i, sName);
        nameCtrl->SetMaxLength(MAXNAMELENGTH);
        nameBox->Add(nameCtrl, 0, wxBOTTOM | wxEXPAND, 3);
    }

    topSizer->Add(nameBox, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);

    /* OK and Cancel buttons */
    wxStdDialogButtonSizer *btnSizer = new wxStdDialogButtonSizer();
    wxButton *okBtn = new wxButton(this, wxID_OK, wxT("OK"));
    okBtn->SetDefault();
    btnSizer->SetAffirmativeButton(okBtn);

    wxButton *cancelBtn = new wxButton(this, wxID_CANCEL, wxT("Cancel"));
    btnSizer->SetCancelButton(cancelBtn);
    btnSizer->Realize();

    topSizer->Add(btnSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizerAndFit(topSizer);
    CentreOnParent();
}


/****************************************************************************

COptionsDlg::OnOK

Save control settings.

****************************************************************************/

void COptionsDlg::OnOK(wxCommandEvent &event)
{
    RegEntry Reg(szRegPath);

    /* Save animation speed setting */
    wxRadioButton *fastBtn = wxDynamicCast(FindWindow(IDC_FAST), wxRadioButton);
    wxRadioButton *slowBtn = wxDynamicCast(FindWindow(IDC_SLOW), wxRadioButton);

    long dwSpeed;
    int nStepSize;

    if (fastBtn && fastBtn->GetValue())
    {
        dwSpeed = IDC_FAST;
        nStepSize = 60;
    }
    else if (slowBtn && slowBtn->GetValue())
    {
        dwSpeed = IDC_SLOW;
        nStepSize = 5;
    }
    else
    {
        dwSpeed = IDC_NORMAL;
        nStepSize = 15;
    }

    card c;
    c.SetStepSize(nStepSize);
    Reg.SetValue(regvalSpeed, dwSpeed);

    /* Save computer player names */
    for (int i = 0; i < 3; i++)
    {
        wxString sDefault = GetStringResource(IDS_P1NAME + i);
        wxTextCtrl *nameCtrl = wxDynamicCast(FindWindow(IDC_NAME1 + i), wxTextCtrl);
        if (!nameCtrl) continue;

        wxString sEdit = nameCtrl->GetValue();

        if (sDefault == sEdit)
            Reg.DeleteValue(regvalPName[i]);
        else
            Reg.SetValue(regvalPName[i], sEdit);
    }

    EndModal(wxID_OK);
}
