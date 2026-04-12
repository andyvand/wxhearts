/* sound.cpp -- wxWidgets port */
/* SoundInit() verifies sound support.
   HeartsPlaySound() plays the sound given a resource id. */

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include <wx/sound.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

static wxSound *s_currentSound = nullptr;


/*
 * SoundInit()
 *
 * returns true if sound is enabled.
 */

bool CMainWindow::SoundInit()
{
    return wxSound::IsPlaying() || true; // assume sound available
}


/*
 * CMainWindow::HeartsPlaySound(id)
 *
 * Plays the specified sound from wav files next to the executable.
 *
 * The application must call HeartsPlaySound(OFF) to stop any
 * currently playing sound before exiting.  (The game destructor
 * does this.  It also happens at the end of each hand.)
 */

bool CMainWindow::HeartsPlaySound(int id)
{
    if (!bHasSound)                 // check for sound capability
        return true;

    if (id == OFF)                  // request to turn off sound
    {
        wxSound::Stop();
        delete s_currentSound;
        s_currentSound = nullptr;
        return true;
    }

    if (!bSoundOn)                  // has user toggled sound off?
        return true;

    // User has requested a sound.  Check if previous sound was freed.

    if (s_currentSound)
    {
        wxSound::Stop();
        delete s_currentSound;
        s_currentSound = nullptr;
    }

#ifdef WXHEARTS_DATADIR
    wxString dir = wxString(wxT(WXHEARTS_DATADIR))
                   + wxFileName::GetPathSeparator();
#else
    wxString dir = wxStandardPaths::Get().GetResourcesDir()
                   + wxFileName::GetPathSeparator();
#endif

    wxString wavFile;
    switch (id)
    {
        case SND_BREAK: wavFile = dir + wxT("glass.wav"); break;
        case SND_QUEEN: wavFile = dir + wxT("timpani.wav"); break;
        case SND_QUOTE: wavFile = dir + wxT("quote2.wav"); break;
        default: return false;
    }

    s_currentSound = new wxSound(wavFile);
    if (s_currentSound->IsOk())
        s_currentSound->Play(wxSOUND_ASYNC);

    return true;
}
