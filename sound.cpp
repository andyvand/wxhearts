/* sound.cpp -- wxWidgets port */
/* SoundInit() verifies sound support.
   HeartsPlaySound() plays the sound given a resource id.
   Sound data is extracted from the compiled XRS archive (hearts.xrs). */

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include <wx/sound.h>
#include <wx/filesys.h>
#include <wx/wfstream.h>
#include <wx/filename.h>

static wxSound *s_currentSound = nullptr;

/* Cached temp file paths for extracted wav data */
static wxString s_wavTempFile[3];
static bool     s_wavExtracted = false;

/*
 * ExtractWavFiles()
 *
 * Extracts wav data from the compiled XRS archive in the memory filesystem
 * to temp files.  Called once on first sound request.
 */

static void ExtractWavFiles()
{
    if (s_wavExtracted)
        return;

    static const wxChar* wavEntries[3] = {
        wxT("hearts.xrs$glass.wav"),
        wxT("hearts.xrs$timpani.wav"),
        wxT("hearts.xrs$quote2.wav")
    };

    wxFileSystem fs;

    for (int i = 0; i < 3; i++) {
        wxString url = wxString(wxT("memory:hearts.xrs#zip:")) + wavEntries[i];

        wxFSFile *fsFile = fs.OpenFile(url);
        if (!fsFile)
            continue;

        wxString tempPath = wxFileName::CreateTempFileName(wxT("wxhearts_snd"));
        wxFileOutputStream out(tempPath);
        out.Write(*fsFile->GetStream());
        out.Close();
        delete fsFile;

        s_wavTempFile[i] = tempPath;
    }

    s_wavExtracted = true;
}


/*
 * SoundInit()
 *
 * returns true if sound is enabled.
 */

bool CMainWindow::SoundInit()
{
#ifndef _WIN32
    return wxSound::IsPlaying() || true; // assume sound available
#else
    return true;
#endif
}


/*
 * CMainWindow::HeartsPlaySound(id)
 *
 * Plays the specified sound extracted from the compiled XRS archive.
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

    // Extract wav files from XRS archive on first use
    ExtractWavFiles();

    wxString wavFile;
    switch (id)
    {
        case SND_BREAK: wavFile = s_wavTempFile[0]; break;
        case SND_QUEEN: wavFile = s_wavTempFile[1]; break;
        case SND_QUOTE: wavFile = s_wavTempFile[2]; break;
        default: return false;
    }

    if (wavFile.IsEmpty())
        return false;

    s_currentSound = new wxSound(wavFile);
    if (s_currentSound->IsOk())
        s_currentSound->Play(wxSOUND_ASYNC);

    return true;
}
