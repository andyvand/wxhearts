/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/****************************************************************************

dlg.h -- wxWidgets port

Dialog classes are declared here.

CScoreDlg       shows current score sheet
CQuoteDlg       quote dialog
CWelcomeDlg     welcome to hearts, do you want to be GameMeister?
COptionsDlg     set options

****************************************************************************/

#ifndef DLG_INC
#define DLG_INC

#include <wx/wx.h>

const int MAXHANDS = 12;
#ifndef MAXPLAYER
#define MAXPLAYER 4
#endif
const int UNKNOWN = -1;

extern "C" {
    extern int nHandsPlayed;
}

class CScoreDlg : public wxDialog
{
public:
    CScoreDlg(wxWindow *pParent);
    CScoreDlg(wxWindow *pParent, int s[MAXPLAYER], int id);
    bool    IsGameOver() { return bGameOver; }
    void    ResetScore() { nHandsPlayed = 0; bGameOver = false; }
    void    SetText();

private:
    void OnPaint(wxPaintEvent &event);
    void OnOK(wxCommandEvent &event);
    bool OnInit();

protected:
    int      m_myid;
    static int  score[MAXPLAYER][MAXHANDS + 1];
    static bool bGameOver;
    wxDECLARE_EVENT_TABLE();
};

class CQuoteDlg : public wxDialog
{
public:
    CQuoteDlg(wxWindow *pParent);
private:
    void OnPaint(wxPaintEvent &event);
    void OnOK(wxCommandEvent &event);
    wxDECLARE_EVENT_TABLE();
};

class CWelcomeDlg : public wxDialog
{
public:
    CWelcomeDlg(wxWindow *pParent);
    wxString GetMyName() { return m_myname; }
    bool     IsGameMeister() { return true; } // always gamemeister in local mode
    bool     IsNetDdeActive() { return false; }

protected:
    void OnOK(wxCommandEvent &event);
    void OnCancel(wxCommandEvent &event);
    wxString m_myname;
    wxDECLARE_EVENT_TABLE();
};

class COptionsDlg : public wxDialog
{
public:
    COptionsDlg(wxWindow *pParent);
protected:
    void OnOK(wxCommandEvent &event);
    wxDECLARE_EVENT_TABLE();
};

#endif
