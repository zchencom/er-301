#include <od/graphics/screensavers/Suns.h>
#include <od/extras/Random.h>
#include <hal/constants.h>
#include <math.h>
#include <string.h>

#define SUNS_SPRITE 36
#define SUNS_SPRITE_W (2 * SUNS_SPRITE + 1)
#define SUNS_MAIN_W 256
#define SUNS_MAIN_H 64
#define SUNS_SUB_W 128
#define SUNS_SUB_H 64
#define SUNS_TWO_PI 6.28318530718f

// Floor on the sub display (y = 0 is the bottom of the screen).
#define SUNS_HORIZON 40 // y of the horizon line

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
      {150.0f, 90.0f, 12.0f, 2.0f, 0.00f, 8.8f, 60.0f},
      {190.0f, 78.0f, 15.0f, 3.0f, 0.33f, 6.4f, -75.0f},
      {235.0f, 100.0f, 10.0f, 2.0f, 0.66f, 4.8f, 45.0f},
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
        if (d > r * 4.0f)
          continue; // nothing visible this far out
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
    float cx[3], cy[3], r[3];
    for (int n = 0; n < 3; n++)
    {
      const SunPath &p = kPaths[n];
      float a = SUNS_TWO_PI * (t / p.period + p.offset);
      cx[n] = 128.0f + p.ax * sinf(a);
      cy[n] = 32.0f + p.ay * sinf(p.yRatio * a);
      r[n] = p.radius * (1.0f + 0.12f * sinf(SUNS_TWO_PI * t / (7.0f + n)));
      float phase = SUNS_TWO_PI * t / p.rayPeriod;
      drawSun(cx[n], cy[n], r[n], phase);
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

    // --- sub display: horizon + one merged pool of light ---
    drawFloor(s, cx, cy, r);
  }

  // 8x8 ordered-dither (Bayer) thresholds. The sub display is 1-bit (a pixel
  // is either on or off), so brightness is shown by the density of lit pixels.
  static const uint8_t kBayer8[64] = {
      0, 32, 8, 40, 2, 34, 10, 42,
      48, 16, 56, 24, 50, 18, 58, 26,
      12, 44, 4, 36, 14, 46, 6, 38,
      60, 28, 52, 20, 62, 30, 54, 22,
      3, 35, 11, 43, 1, 33, 9, 41,
      51, 19, 59, 27, 49, 17, 57, 25,
      15, 47, 7, 39, 13, 45, 5, 37,
      63, 31, 55, 23, 61, 29, 53, 21};

  void Suns::drawFloor(FrameBuffer &s, const float *cx, const float *cy,
                       const float *r)
  {
    // Each sun throws a soft patch of light through an imagined opening onto
    // the floor. The patch moves opposite to the sun (light comes "through"
    // the screen), lands closer to the viewer when the sun is higher, and is
    // brighter for a bigger sun. The three patches are added together, so they
    // merge into one pool of light whose shape keeps changing.
    // One pool of light: centred on the brightness-weighted centre of the three
    // suns (mirrored, as if the light comes through the screen), wider when the
    // suns are spread apart, closer to the viewer when they are high. Each sun
    // only adds a faint lobe inside the pool, so the three read as one.
    float w[3];
    float total = 0.0f, mx = 0.0f, my = 0.0f;
    for (int n = 0; n < 3; n++)
    {
      w[n] = r[n] * r[n];
      total += w[n];
      mx += w[n] * cx[n];
      my += w[n] * cy[n];
    }
    mx /= total;
    my /= total;
    float varx = 0.0f;
    for (int n = 0; n < 3; n++)
    {
      float d = cx[n] - mx;
      varx += w[n] * d * d;
    }
    float spreadSuns = sqrtf(varx / total); // 0..~90 px on the main display

    float pcx = 64.0f - 0.42f * (mx - 128.0f);
    float pcy = 6.0f + (52.0f - my) * 0.55f;
    if (pcy < 4.0f)
      pcy = 4.0f;
    if (pcy > SUNS_HORIZON - 9.0f)
      pcy = SUNS_HORIZON - 9.0f;
    float poolW = 1.0f + spreadSuns / 60.0f; // wider pool for spread-out suns

    float px[3], py[3];
    for (int n = 0; n < 3; n++)
    {
      // lobes pulled most of the way into the pool's centre
      px[n] = pcx + 0.22f * (-0.42f * (cx[n] - mx));
      py[n] = pcy + 0.15f * (my - cy[n]) * 0.55f;
    }

    for (int y = 0; y < SUNS_HORIZON; y++)
    {
      // perspective: things near the viewer (small y) are wider
      float depth = (float)(SUNS_HORIZON - y) / SUNS_HORIZON; // 1 near, 0 far
      float spread = 7.0f + 16.0f * depth;
      // ambient light falling off towards the horizon
      float ambient = 0.015f + 0.035f * depth;
      for (int x = 0; x < SUNS_SUB_W; x++)
      {
        // main pool
        float dx = ((float)x - pcx) / (spread * poolW);
        float dy = ((float)y - pcy) / (5.0f + 6.0f * depth);
        float e = dx * dx + dy * dy;
        float v = 0.0f;
        if (e < 9.0f)
        {
          v = 0.92f * expf(-e * e * 0.35f);
        }
        // faint lobes from the individual suns, inside the pool
        for (int n = 0; n < 3; n++)
        {
          float lx = ((float)x - px[n]) / (spread * 0.55f);
          float ly = ((float)y - py[n]) / (4.0f + 4.0f * depth);
          float le = lx * lx + ly * ly;
          if (le < 6.0f)
          {
            v += 0.18f * (w[n] / total) * 3.0f * expf(-le);
          }
        }
        v += ambient;
        if (v > 1.0f)
          v = 1.0f;
        // lit if brighter than this pixel's dither threshold
        float threshold = ((float)kBayer8[(y & 7) * 8 + (x & 7)] + 0.5f) / 64.0f;
        if (v > threshold)
          s.pixel(WHITE, x, y);
      }
    }

    // horizon: a dotted line
    for (int x = 0; x < SUNS_SUB_W; x += 2)
    {
      s.pixel(WHITE, x, SUNS_HORIZON);
    }
  }

} /* namespace od */
