/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991, 1992                   **/
/***************************************************************************/

/* resource.h -- wxWidgets port */

#ifndef __RESOURCE_H__
#define __RESOURCE_H__

/* Menu / command IDs */
#define IDM_ABOUT       101
#define IDM_NEWGAME     102
#define IDM_EXIT        103
#define IDM_HELP        106
#define IDM_BOSSKEY     114
#define IDM_BUTTON      115
#define IDM_SHOWBUTTON  116
#define IDM_HIDEBUTTON  117
#define IDM_REF         123
#define IDM_SOUND       124
#define IDM_SCORE       125
#define IDM_WELCOME     126
#define IDM_QUOTE       127
#define IDM_OPTIONS     128
#define IDM_CHEAT       121

/* Dialog control IDs */
#define IDC_YOURNAME    201
#define IDC_SLOW        215
#define IDC_NORMAL      216
#define IDC_FAST        217
#define IDC_NAME1       218
#define IDC_NAME2       219
#define IDC_NAME3       220

/* String IDs -- used as indices into GetStringResource() */
#define IDS_MEMORY      301
#define IDS_APPNAME     302
#define IDS_P1NAME      303
#define IDS_P2NAME      304
#define IDS_P3NAME      305
#define IDS_INTRO       306
#define IDS_SELECT      307
#define IDS_CONNECTING  308
#define IDS_PASSWAIT    309
#define IDS_WAIT1       310
#define IDS_GO          311
#define IDS_BADMOVE     312
#define IDS_LEADHEARTS  313
#define IDS_LEAD2C      314
#define IDS_ACCEPT      315
#define IDS_OK          316
#define IDS_SCORE       317
#define IDS_SCORESHEET  318
#define IDS_PLACE1      319
#define IDS_PLACE2      320
#define IDS_PLACE3      321
#define IDS_PLACE4      322
#define IDS_WAIT2       341

#define IDS_DISCONNECT  323
#define IDS_NOSERVER    324
#define IDS_SERVERFAIL  325
#define IDS_CARDSDLL    326
#define IDS_VERSION     327
#define IDS_PWAIT       328
#define IDS_GMWAIT      329
#define IDS_BUSY        330
#define IDS_TIMEOUT     331
#define IDS_UNKNOWNERR  332
#define IDS_GAMEOVER    333
#define IDS_GAMEOVERWIN 334
#define IDS_NETWORK     335
#define IDS_NOTREADY    336
#define IDS_AGAIN       337
#define IDS_BADBLOOD    338
#define IDS_UNKNOWN     339
#define IDS_DEALER      340

#define IDS_CREDITS     399

#define IDS_SUIT0       401
#define IDS_SUIT1       402
#define IDS_SUIT2       403
#define IDS_SUIT3       404

#define IDS_PASSLEFT    405
#define IDS_PASSRIGHT   406
#define IDS_PASSACROSS  407

#define IDS_FONTFACE    410
#define IDS_CHARSET     411
#define IDS_FONTSIZE    412

#define SND_BREAK       401
#define SND_QUEEN       402
#define SND_QUOTE       403

/* String table -- replaces .rc STRINGTABLE */

#include <wx/string.h>
#include <map>

inline wxString GetStringResource(int id)
{
    static const std::map<int, wxString> strings = {
        {IDS_MEMORY,      wxT("Out of memory. Close some other applications and try again.")},
        {IDS_APPNAME,     wxT("The Microsoft Hearts Network")},
        {IDS_CONNECTING,  wxT("Trying to connect with dealer...")},
        {IDS_P1NAME,      wxT("Pauline")},
        {IDS_P2NAME,      wxT("Michele")},
        {IDS_P3NAME,      wxT("Ben")},
        {IDS_INTRO,       wxT("Welcome to the Microsoft Hearts Network.")},
        {IDS_SELECT,      wxT("Select three cards to pass to")},
        {IDS_PASSWAIT,    wxT("Waiting for other players to pass...")},
        {IDS_WAIT1,       wxT("Waiting for ")},
        {IDS_WAIT2,       wxT(" to move...")},
        {IDS_GO,          wxT("Select a card to play.")},
        {IDS_ACCEPT,      wxT("Press OK to accept cards.")},
        {IDS_OK,          wxT("OK")},
        {IDS_BADMOVE,     wxT("You must follow suit.  Play a")},
        {IDS_LEADHEARTS,  wxT("Hearts has not been broken.  Choose another suit.")},
        {IDS_LEAD2C,      wxT("You must lead the two of clubs.")},
        {IDS_SCORE,       wxT("Score")},
        {IDS_SCORESHEET,  wxT("Score Sheet")},
        {IDS_PLACE1,      wxT("First Place")},
        {IDS_PLACE2,      wxT("Second Place")},
        {IDS_PLACE3,      wxT("Third Place")},
        {IDS_PLACE4,      wxT("Last Place")},
        {IDS_DISCONNECT,  wxT("The dealer has left the game.\nHearts will end.")},
        {IDS_NOSERVER,    wxT("Unable to connect with dealer.\nHearts will end.")},
        {IDS_SERVERFAIL,  wxT("Unable to establish server.\nHearts will end.")},
        {IDS_CARDSDLL,    wxT("Cannot find card bitmaps.\nHearts will end.")},
        {IDS_VERSION,     wxT("Incompatible version.  Hearts will end.")},
        {IDS_PWAIT,       wxT("Waiting for dealer to start game...")},
        {IDS_GMWAIT,      wxT("Waiting for others to join...   Press F2 to begin with current players.")},
        {IDS_BUSY,        wxT("Sorry, game is already in progress.")},
        {IDS_TIMEOUT,     wxT("Network timeout error.\nHearts will end.")},
        {IDS_UNKNOWNERR,  wxT("Network communication error.\nHearts will end.")},
        {IDS_GAMEOVER,    wxT("Game Over")},
        {IDS_GAMEOVERWIN, wxT("Game Over -- You Win")},
        {IDS_NETWORK,     wxT("Hearts Network")},
        {IDS_NOTREADY,    wxT("The dealer is not ready, or the game is already in progress.")},
        {IDS_AGAIN,       wxT("Do you want to play again?")},
        {IDS_BADBLOOD,    wxT("You cannot play a point card on the first trick.  Select again.")},
        {IDS_SUIT0,       wxT("club")},
        {IDS_SUIT1,       wxT("diamond")},
        {IDS_SUIT2,       wxT("heart")},
        {IDS_SUIT3,       wxT("spade")},
        {IDS_PASSLEFT,    wxT("Pass Left")},
        {IDS_PASSRIGHT,   wxT("Pass Right")},
        {IDS_PASSACROSS,  wxT("Pass Across")},
        {IDS_UNKNOWN,     wxT("Unknown")},
        {IDS_DEALER,      wxT("Dealer")},
        {IDS_CREDITS,     wxT("I come not, friends, to steal away your hearts...\n\n\tJulius Caesar, Act III, Scene ii")},
        {IDS_FONTFACE,    wxT("Tahoma")},
        {IDS_CHARSET,     wxT("0")},
        {IDS_FONTSIZE,    wxT("13")},
    };

    auto it = strings.find(id);
    if (it != strings.end())
        return it->second;
    return wxT("");
}

#endif /* __RESOURCE_H__ */
