/* sound_retro68.cpp -- classic Mac OS (Retro68) sound for wxhearts.

   wxRetro68 has no wxSound (wxUSE_SOUND=0), so the sounds are played with
   the Sound Manager instead.  The same WAV files as on the other platforms
   are read from the compiled XRS archive in the memory filesystem, turned
   into format 1 'snd ' sampled sounds in memory once, and played
   asynchronously on a sound channel of our own.  The WAVs are 8-bit mono
   PCM, whose unsigned samples are what a standard sound header holds. */

#include "hearts.h"
#include "main.h"
#include "resource.h"
#include <wx/filesys.h>
#include <wx/stream.h>

#include <Memory.h>
#include <Sound.h>

#include <string.h>
#include <vector>

namespace
{

Handle        s_snd[3]  = { nullptr, nullptr, nullptr };
bool          s_loaded  = false;
SndChannelPtr s_channel = nullptr;

inline unsigned le16(const unsigned char *p)
{
    return p[0] | (p[1] << 8);
}

inline unsigned long le32(const unsigned char *p)
{
    return (unsigned long)p[0] | ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) | ((unsigned long)p[3] << 24);
}

inline void put16(unsigned char *&p, unsigned v)
{
    *p++ = (unsigned char)(v >> 8);
    *p++ = (unsigned char)v;
}

inline void put32(unsigned char *&p, unsigned long v)
{
    *p++ = (unsigned char)(v >> 24);
    *p++ = (unsigned char)(v >> 16);
    *p++ = (unsigned char)(v >> 8);
    *p++ = (unsigned char)v;
}

/* Builds a format 1 'snd ' from 8-bit mono PCM WAV data, nullptr if the
   WAV is anything else. */
Handle MakeSnd(const std::vector<unsigned char> &wav)
{
    const size_t n = wav.size();
    if (n < 12 || memcmp(&wav[0], "RIFF", 4) != 0 || memcmp(&wav[8], "WAVE", 4) != 0)
        return nullptr;

    unsigned long rate = 0;
    unsigned channels = 0, bits = 0, format = 0;
    const unsigned char *samples = nullptr;
    unsigned long count = 0;

    for (size_t pos = 12; pos + 8 <= n; )
    {
        const unsigned char *chunk = &wav[pos];
        unsigned long size = le32(chunk + 4);
        if (pos + 8 + size > n)
            size = n - pos - 8;
        if (memcmp(chunk, "fmt ", 4) == 0 && size >= 16)
        {
            format   = le16(chunk + 8);
            channels = le16(chunk + 10);
            rate     = le32(chunk + 12);
            bits     = le16(chunk + 22);
        }
        else if (memcmp(chunk, "data", 4) == 0)
        {
            samples = chunk + 8;
            count   = size;
        }
        pos += 8 + size + (size & 1);
    }

    if (format != 1 || channels != 1 || bits != 8 || !samples || !count || !rate)
        return nullptr;

    /* Header: format, 1 data format (sampledSynth, mono), 1 command
       (bufferCmd with dataOffsetFlag, pointing at the sound header), then
       the standard sound header and the samples. */
    const long headerOffset = 20;
    const long total = headerOffset + 22 + (long)count;
    Handle h = NewHandle(total);
    if (!h)
        return nullptr;

    HLock(h);
    unsigned char *p = (unsigned char *)*h;
    put16(p, 1);                        // format 1
    put16(p, 1);                        // one data format
    put16(p, sampledSynth);
    put32(p, 0x0080);                   // initMono
    put16(p, 1);                        // one command
    put16(p, 0x8000 | bufferCmd);       // dataOffsetFlag | bufferCmd
    put16(p, 0);                        // param1
    put32(p, headerOffset);             // param2: offset of the header
    put32(p, 0);                        // samplePtr: samples follow
    put32(p, count);                    // length
    put32(p, rate << 16);               // sampleRate (Fixed)
    put32(p, 0);                        // loopStart
    put32(p, 0);                        // loopEnd
    *p++ = stdSH;                       // encode
    *p++ = 60;                          // baseFrequency: middle C
    memcpy(p, samples, count);
    HUnlock(h);
    return h;
}

void LoadSounds()
{
    if (s_loaded)
        return;
    s_loaded = true;

    static const wxChar *entries[3] = {
        wxT("hearts.xrs$glass.wav"),
        wxT("hearts.xrs$timpani.wav"),
        wxT("hearts.xrs$quote2.wav")
    };

    wxFileSystem fs;
    for (int i = 0; i < 3; i++)
    {
        wxFSFile *file = fs.OpenFile(wxString(wxT("memory:hearts.xrs#zip:")) + entries[i]);
        if (!file)
            continue;

        std::vector<unsigned char> wav;
        wxInputStream *in = file->GetStream();
        unsigned char buf[4096];
        while (in && !in->Eof())
        {
            in->Read(buf, sizeof(buf));
            size_t got = in->LastRead();
            if (!got)
                break;
            wav.insert(wav.end(), buf, buf + got);
        }
        delete file;

        s_snd[i] = MakeSnd(wav);
    }
}

void StopHeartsSound()
{
    if (s_channel)
    {
        SndDisposeChannel(s_channel, true);     // quiet now
        s_channel = nullptr;
    }
}

} // anonymous namespace


/*
 * SoundInit()
 *
 * returns true if sound is enabled.  Every Mac has the Sound Manager.
 */

bool CMainWindow::SoundInit()
{
    return true;
}


/*
 * CMainWindow::HeartsPlaySound(id)
 *
 * Plays the specified sound; HeartsPlaySound(OFF) stops the one playing.
 */

bool CMainWindow::HeartsPlaySound(int id)
{
    if (!bHasSound)
        return true;

    if (id == OFF)
    {
        StopHeartsSound();
        return true;
    }

    if (!bSoundOn)
        return true;

    int index;
    switch (id)
    {
        case SND_BREAK: index = 0; break;
        case SND_QUEEN: index = 1; break;
        case SND_QUOTE: index = 2; break;
        default: return false;
    }

    LoadSounds();
    if (!s_snd[index])
        return false;

    // A fresh channel each time: disposing of the old one is the simplest
    // way to cut off a sound still playing.
    StopHeartsSound();
    if (SndNewChannel(&s_channel, sampledSynth, 0x0080, nullptr) != noErr)
    {
        s_channel = nullptr;
        return false;
    }

    HLock(s_snd[index]);
    if (SndPlay(s_channel, s_snd[index], true) != noErr)
    {
        HUnlock(s_snd[index]);
        StopHeartsSound();
        return false;
    }
    // The handle stays locked: the Sound Manager plays from it at interrupt
    // time, and the sounds live for the whole run anyway.
    return true;
}
