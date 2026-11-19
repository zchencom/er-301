#include <od/graphics/screensavers/Wavs.h>
#include <od/graphics/constants.h>
#include <hal/constants.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace od
{

  struct WavFile
  {
    const char *name;
    unsigned long sector; // where PhotoRec found it (the number in the name)
    float seconds;        // length of the recording
  };

  // Ordered by position on the card, as in the folder listing.
  static const WavFile kWavs[13] = {
      {"f1148416.wav", 1148416UL, 24.6f},
      {"f1278400.wav", 1278400UL, 2.5f},
      {"f1990720.wav", 1990720UL, 3.1f},
      {"f2730624.wav", 2730624UL, 12.6f},
      {"f3146048.wav", 3146048UL, 2.1f},
      {"f3357632.wav", 3357632UL, 33.3f},
      {"f3530624.wav", 3530624UL, 35.5f},
      {"f6233728.wav", 6233728UL, 46.7f},
      {"f12418496.wav", 12418496UL, 2.6f},
      {"f13928064.wav", 13928064UL, 2.8f},
      {"f14040768.wav", 14040768UL, 2.6f},
      {"f14260096.wav", 14260096UL, 5.0f},
      {"f14862016.wav", 14862016UL, 3.1f},
  };
  static const unsigned long kCardSectors = 15523840UL; // 7948 MB card
  static const float kGap = 3.0f;                         // pause after a pass

  Wavs::Wavs()
  {
    reset();
  }

  Wavs::~Wavs()
  {
  }

  void Wavs::reset()
  {
    mTime = 0.0f;
    mCurrent = 0;
    mPause = 0.0f;
    mDrift = 0.0f;
    for (int i = 0; i < 13; i++)
      mFade[i] = 0.0f;
  }

  static void fmtTime(char *buf, int len, float s)
  {
    int t = (int)s;
    snprintf(buf, len, "%d:%02d", t / 60, t % 60);
  }

  void Wavs::draw(FrameBuffer &m, FrameBuffer &s)
  {
    const float dt = GRAPHICS_REFRESH_PERIOD;

    // ---- advance the silent playhead ----
    for (int i = 0; i < 13; i++)
    {
      mFade[i] -= dt / 4.0f; // afterglow lasts ~4 s
      if (mFade[i] < 0.0f)
        mFade[i] = 0.0f;
    }
    if (mPause > 0.0f)
    {
      mPause -= dt;
    }
    else
    {
      mTime += dt;
      if (mTime >= kWavs[mCurrent].seconds)
      {
        mFade[mCurrent] = 1.0f;
        mTime = 0.0f;
        mCurrent++;
        if (mCurrent >= 13)
        {
          mCurrent = 0;
          mPause = kGap;
        }
      }
    }
    bool playing = mPause <= 0.0f;

    // ---- main display: the folder, 3 columns x 5 rows ----
    mDrift += dt;
    int dx = (int)lroundf(3.0f * sinf(mDrift * 0.021f));
    int dy = (int)lroundf(2.0f * sinf(mDrift * 0.017f + 1.0f));
    const int colX[3] = {10, 94, 178};
    for (int i = 0; i < 13; i++)
    {
      int col = i % 3, row = i / 3;
      int x = colX[col] + dx;
      int y = 52 - row * 12 + dy; // baseline, y = 0 is the bottom
      int c = GRAY3;
      int fade = (int)(mFade[i] * 8.0f);
      if (GRAY3 + fade > c)
        c = GRAY3 + fade;
      if (playing && i == mCurrent)
        c = WHITE;
      m.text(c, x, y, kWavs[i].name, 10);
      if (playing && i == mCurrent)
      {
        // progress line under the name
        int w = (int)(76.0f * mTime / kWavs[i].seconds);
        m.hline(GRAY5, x, x + 76, y - 3, 2);
        if (w > 0)
          m.hline(GRAY12, x, x + w, y - 3);
      }
    }

    // ---- sub display (1-bit): the file now, and where it lay ----
    const WavFile &f = kWavs[mCurrent];
    char buf[40];
    if (playing)
    {
      int w = (int)strlen(f.name) * 7; // consolas 12 is 7 px per character
      s.text(WHITE, 64 - w / 2, 48, f.name, 12);
    }
    else
    {
      s.text(WHITE, 64 - 9 * 7 / 2, 48, "13 sounds", 12);
    }

    snprintf(buf, sizeof(buf), "sector %lu", f.sector);
    s.text(WHITE, 4, 38, buf, 8);
    unsigned long mb = (unsigned long)((f.sector * 512.0) / 1000000.0 + 0.5);
    snprintf(buf, sizeof(buf), "%lu MB into the card", mb);
    s.text(WHITE, 4, 29, buf, 8);

    // elapsed / length
    char a[12], b[12];
    fmtTime(a, sizeof(a), playing ? mTime : 0.0f);
    fmtTime(b, sizeof(b), f.seconds);
    snprintf(buf, sizeof(buf), "%s / %s", a, b);
    s.text(WHITE, 4, 18, buf, 8);

    // map of the card: a line, a tick for every file, a tall one for this one
    s.hline(WHITE, 4, 123, 6, 2);
    for (int i = 0; i < 13; i++)
    {
      int x = 4 + (int)(119.0f * (float)kWavs[i].sector / (float)kCardSectors);
      int h = (i == mCurrent && playing) ? 5 : 2;
      s.vline(WHITE, x, 6, 6 + h);
    }
  }

} /* namespace od */
