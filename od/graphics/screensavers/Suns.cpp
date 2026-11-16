#include <od/graphics/screensavers/Suns.h>
#include <od/graphics/screensavers/NameBitmap.h>
#include <od/extras/Random.h>
#include <hal/constants.h>
#include <math.h>
#include <string.h>

#define SUNS_SPRITE 24
#define SUNS_SPRITE_W (2 * SUNS_SPRITE + 1)
#define SUNS_MAIN_W 256
#define SUNS_MAIN_H 64
#define SUNS_SUB_W 128
#define SUNS_SUB_H 64
#define SUNS_TWO_PI 6.28318530718f

// Scrolling name on the sub display.
#define SUNS_SCROLL_SPEED 16.0f // pixels per second
#define SUNS_SCROLL_GAP 48      // empty pixels between repeats

namespace od
{

  struct SunPath
  {
    float period;    // seconds for one loop of the path
    float ax, ay;    // amplitude (pixels)
    float yRatio;    // vertical frequency relative to horizontal
    float offset;    // phase offset (0..1)
    float radius;    // disc radius (pixels)
    float rayPeriod; // seconds per ray rotation (negative = other way)
  };

  // Three suns of different sizes, each on its own slow path.
  static const SunPath kPaths[3] = {
      {150.0f, 95.0f, 14.0f, 2.0f, 0.00f, 5.5f, 60.0f},
      {190.0f, 80.0f, 18.0f, 3.0f, 0.33f, 4.0f, -75.0f},
      {235.0f, 105.0f, 11.0f, 2.0f, 0.66f, 3.0f, 45.0f},
  };

  Suns::Suns()
  {
    mDist.resize(SUNS_SPRITE_W * SUNS_SPRITE_W);
    mAngle.resize(SUNS_SPRITE_W * SUNS_SPRITE_W);
    for (int j = 0; j < SUNS_SPRITE_W; j++)
    {
      for (int i = 0; i < SUNS_SPRITE_W; i++)
      {
        float dx = (float)(i - SUNS_SPRITE);
        float dy = (float)(j - SUNS_SPRITE);
        mDist[j * SUNS_SPRITE_W + i] = sqrtf(dx * dx + dy * dy);
        mAngle[j * SUNS_SPRITE_W + i] = atan2f(dy, dx);
      }
    }
    mCanvas.assign(SUNS_MAIN_W * SUNS_MAIN_H, 0);
  }

  Suns::~Suns()
  {
  }

  void Suns::reset()
  {
    t = Random::generateFloat(0.0f, 600.0f);
  }

  void Suns::drawSun(float cx, float cy, float r, float phase)
  {
    int icx = (int)(cx + 0.5f);
    int icy = (int)(cy + 0.5f);
    for (int j = 0; j < SUNS_SPRITE_W; j++)
    {
      int y = icy + j - SUNS_SPRITE;
      if (y < 0 || y >= SUNS_MAIN_H)
        continue;
      for (int i = 0; i < SUNS_SPRITE_W; i++)
      {
        int x = icx + i - SUNS_SPRITE;
        if (x < 0 || x >= SUNS_MAIN_W)
          continue;
        int k = j * SUNS_SPRITE_W + i;
        float d = mDist[k];
        float v;
        if (d <= r)
        {
          v = 1.0f;
        }
        else
        {
          v = 0.35f * expf(-(d - r) / (r * 0.6f));
          if (d < r * 4.0f)
          {
            float c = cosf(8.0f * mAngle[k] + phase);
            if (c > 0.0f)
            {
              float c2 = c * c;
              float c4 = c2 * c2;
              float rv = c4 * c4 * 0.9f * (1.0f - (d - r) / (r * 3.0f));
              if (rv > v)
                v = rv;
            }
          }
        }
        if (v <= 0.0f)
          continue;
        int color = v >= 1.0f ? WHITE : (int)(v * WHITE + 0.5f);
        uint8_t &px = mCanvas[y * SUNS_MAIN_W + x];
        if (color > px)
          px = (uint8_t)color; // brightest sun wins where they overlap
      }
    }
  }

  void Suns::draw(FrameBuffer &m, FrameBuffer &s)
  {
    t += GRAPHICS_REFRESH_PERIOD;
    if (t > 24 * 3600)
    {
      t = 0.0f;
    }

    // --- main display: three suns ---
    memset(mCanvas.data(), 0, mCanvas.size());
    for (int n = 0; n < 3; n++)
    {
      const SunPath &p = kPaths[n];
      float a = SUNS_TWO_PI * (t / p.period + p.offset);
      float cx = 128.0f + p.ax * sinf(a);
      float cy = 32.0f + p.ay * sinf(p.yRatio * a);
      float r = p.radius * (1.0f + 0.12f * sinf(SUNS_TWO_PI * t / (7.0f + n)));
      float phase = SUNS_TWO_PI * t / p.rayPeriod;
      drawSun(cx, cy, r, phase);
    }
    for (int y = 0; y < SUNS_MAIN_H; y++)
    {
      const uint8_t *row = &mCanvas[y * SUNS_MAIN_W];
      for (int x = 0; x < SUNS_MAIN_W; x++)
      {
        if (row[x])
          m.pixel(row[x], x, y);
      }
    }

    // --- sub display: the name scrolls from right to left, forever ---
    const int loop = NAME_BITMAP_W + SUNS_SCROLL_GAP;
    int shift = ((int)(t * SUNS_SCROLL_SPEED)) % loop;
    // y = 0 is the bottom of the display; bitmap row 0 is the top of the text
    int top = (SUNS_SUB_H + NAME_BITMAP_H) / 2 - 1;
    for (int x0 = SUNS_SUB_W - shift; x0 < SUNS_SUB_W; x0 += loop)
    {
      for (int r = 0; r < NAME_BITMAP_H; r++)
      {
        int y = top - r;
        const uint8_t *row = &NAME_BITMAP[r * NAME_BITMAP_W];
        for (int c = 0; c < NAME_BITMAP_W; c++)
        {
          int x = x0 + c;
          if (x < 0 || x >= SUNS_SUB_W || row[c] == 0)
            continue;
          s.pixel(row[c], x, y);
        }
      }
    }
    // the copy entering from the left edge, so the loop is seamless
    for (int x0 = SUNS_SUB_W - shift - loop; x0 + NAME_BITMAP_W > 0; x0 -= loop)
    {
      for (int r = 0; r < NAME_BITMAP_H; r++)
      {
        int y = top - r;
        const uint8_t *row = &NAME_BITMAP[r * NAME_BITMAP_W];
        for (int c = 0; c < NAME_BITMAP_W; c++)
        {
          int x = x0 + c;
          if (x < 0 || x >= SUNS_SUB_W || row[c] == 0)
            continue;
          s.pixel(row[c], x, y);
        }
      }
    }
  }

} /* namespace od */
