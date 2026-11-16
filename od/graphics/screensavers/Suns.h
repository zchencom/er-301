#pragma once

#include <od/graphics/ScreenSaver.h>
#include <stdint.h>
#include <vector>

namespace od
{

  // Three small suns wandering across the main display on their own slow
  // Lissajous paths, each with its own size and ray rotation. The sub display
  // scrolls a name, rendered as a 4-bit bitmap (see NameBitmap.h).
  class Suns : public ScreenSaver
  {
  public:
    Suns();
    virtual ~Suns();

    virtual void reset();
    virtual void draw(FrameBuffer &mainFrameBuffer,
                      FrameBuffer &subFrameBuffer);

  private:
    void drawSun(float cx, float cy, float r, float phase);

    float t = 0.0f;
    std::vector<float> mDist;     // sprite offset -> distance from centre
    std::vector<float> mAngle;    // sprite offset -> angle
    std::vector<uint8_t> mCanvas; // 256 x 64 brightness, so suns can overlap
  };

} /* namespace od */
