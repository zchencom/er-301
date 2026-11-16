#include <od/graphics/screensavers/Sun.h>
#include <od/extras/Random.h>
#include <hal/constants.h>
#include <math.h>

// Half-size of the square area around the sun that we render (pixels).
#define SUN_SPRITE 24
#define SUN_SPRITE_W (2 * SUN_SPRITE + 1)

// Timing (seconds).
#define SUN_PATH_PERIOD 120.0f   // one full figure-of-eight across the screen
#define SUN_RAY_PERIOD 60.0f     // one full rotation of the rays
#define SUN_BREATH_PERIOD 8.0f   // radius breathing

#define SUN_TWO_PI 6.28318530718f

namespace od
{

  Sun::Sun()
  {
    // Pre-compute distance and angle for every pixel of the sprite once,
    // so each frame only needs a few multiplies per pixel.
    mDist.resize(SUN_SPRITE_W * SUN_SPRITE_W);
    mAngle.resize(SUN_SPRITE_W * SUN_SPRITE_W);
    for (int j = 0; j < SUN_SPRITE_W; j++)
    {
      for (int i = 0; i < SUN_SPRITE_W; i++)
      {
        float dx = (float)(i - SUN_SPRITE);
        float dy = (float)(j - SUN_SPRITE);
        mDist[j * SUN_SPRITE_W + i] = sqrtf(dx * dx + dy * dy);
        mAngle[j * SUN_SPRITE_W + i] = atan2f(dy, dx);
      }
    }
  }

  Sun::~Sun()
  {
  }

  void Sun::reset()
  {
    // Start somewhere different on the path each time the saver kicks in.
    t = Random::generateFloat(0.0f, SUN_PATH_PERIOD);
  }

  static inline int toColor(float v)
  {
    if (v <= 0.0f)
      return 0;
    if (v >= 1.0f)
      return WHITE;
    return (int)(v * WHITE + 0.5f);
  }

  void Sun::draw(FrameBuffer &m, FrameBuffer &s)
  {
    t += GRAPHICS_REFRESH_PERIOD;
    if (t > 24 * 3600)
    {
      t = 0.0f;
    }

    // --- main display (256 x 64): the drifting sun ---
    float path = SUN_TWO_PI * t / SUN_PATH_PERIOD;
    float cx = 128.0f + 70.0f * sinf(path);
    float cy = 32.0f + 10.0f * sinf(2.0f * path);
    float r = 5.0f + 0.8f * sinf(SUN_TWO_PI * t / SUN_BREATH_PERIOD);
    float phase = SUN_TWO_PI * t / SUN_RAY_PERIOD;

    int icx = (int)(cx + 0.5f);
    int icy = (int)(cy + 0.5f);

    for (int j = 0; j < SUN_SPRITE_W; j++)
    {
      for (int i = 0; i < SUN_SPRITE_W; i++)
      {
        int k = j * SUN_SPRITE_W + i;
        float d = mDist[k];
        float v;
        if (d <= r)
        {
          v = 1.0f; // solid disc
        }
        else
        {
          // soft glow
          v = 0.35f * expf(-(d - r) / (r * 0.6f));
          // 8 rotating rays, fading out with distance
          if (d < r * 4.0f)
          {
            float c = cosf(8.0f * mAngle[k] + phase);
            if (c > 0.0f)
            {
              float c2 = c * c;
              float c4 = c2 * c2;
              float ray = c4 * c4; // c^8: thin rays
              float rv = ray * 0.9f * (1.0f - (d - r) / (r * 3.0f));
              if (rv > v)
                v = rv;
            }
          }
        }
        int color = toColor(v);
        if (color > 0)
        {
          m.pixel(color, icx + i - SUN_SPRITE, icy + j - SUN_SPRITE);
        }
      }
    }

    // --- sub display (128 x 64): horizon + light on the floor ---
    // dotted horizon
    for (int x = 0; x < 128; x += 3)
    {
      s.pixel(5, x, 10);
    }
    // the patch of light the sun throws on the floor moves the opposite way,
    // as if the light came through a hole in a wall.
    float px = 64.0f - 0.45f * (cx - 128.0f);
    float pw = 6.0f + 3.0f * (cy - 22.0f) / 20.0f; // higher sun -> wider patch
    for (int y = 2; y < 9; y++)
    {
      float spread = pw * (1.0f + (8 - y) * 0.06f); // slight perspective
      int x0 = (int)(px - spread);
      int x1 = (int)(px + spread);
      for (int x = x0; x <= x1; x++)
      {
        // soft edges
        float e = 1.0f - fabsf((float)x - px) / (spread + 1.0f);
        s.pixel(toColor(0.25f + 0.6f * e), x, y);
      }
    }
  }

} /* namespace od */
