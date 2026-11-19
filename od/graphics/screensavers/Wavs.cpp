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
    mScroll = 0.0f;
  }

  // deterministic pseudo-random value in [0,1) for (seed, i)
  static float hash01(unsigned seed, int i)
  {
    unsigned h = seed * 0x45d9f3bu + (unsigned)i * 0x27d4eb2du;
    h ^= h >> 15;
    h *= 0x2c1b3c6du;
    h ^= h >> 12;
    h *= 0x297a2d39u;
    h ^= h >> 15;
    return (float)(h & 0xffffff) / 16777216.0f;
  }

  void Wavs::draw(FrameBuffer &m, FrameBuffer &s)
  {
    const float dt = GRAPHICS_REFRESH_PERIOD;

    // ---- advance the silent playhead ----
    if (mPause > 0.0f)
    {
      mPause -= dt;
    }
    else
    {
      mTime += dt;
      if (mTime >= kWavs[mCurrent].seconds)
      {
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

    // ---- main display: the folder, 3 columns x 5 rows, all lit ----
    mDrift += dt;
    int dx = (int)lroundf(3.0f * sinf(mDrift * 0.021f));
    int dy = (int)lroundf(2.0f * sinf(mDrift * 0.017f + 1.0f));
    const int colX[3] = {10, 94, 178};
    for (int i = 0; i < 13; i++)
    {
      int col = i % 3, row = i / 3;
      int c = WHITE;
      m.text(c, colX[col] + dx, 52 - row * 12 + dy, kWavs[i].name, 10);
    }

    // ---- sub display (1-bit): a waveform, drawn like an audio editor ----
    mScroll += dt * 30.0f; // pixels per second
    if (mScroll > 1000000.0f)
      mScroll -= 1000000.0f;
    int off = (int)mScroll;
    unsigned seed = (unsigned)kWavs[mCurrent].sector;
    for (int x = 0; x < 128; x++)
    {
      int a = 0;
      if (playing)
      {
        int u = x + off;
        // slow envelope: smooth value noise, ~24 px per bump
        int k = u / 24;
        float f = (float)(u % 24) / 24.0f;
        f = f * f * (3.0f - 2.0f * f);
        float e0 = hash01(seed, k), e1 = hash01(seed, k + 1);
        float env = e0 + (e1 - e0) * f;
        env = env * env;
        // fast detail, different on every column
        float fine = 0.35f + 0.65f * hash01(seed ^ 0x9e3779b9u, u);
        a = (int)(28.0f * env * fine);
      }
      s.vline(WHITE, x, 32 - a, 32 + a);
    }
  }

} /* namespace od */
