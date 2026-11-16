#pragma once

#include <od/graphics/ScreenSaver.h>
#include <stdint.h>
#include <vector>

namespace od
{

  // Three small suns wandering across the main display on their own slow
  // Lissajous paths, each with its own size and ray rotation. The sub display
  // shows a horizon and a single pool of light on the floor: the three suns'
  // light merged into one soft, perspective-correct patch that morphs as the
  // suns move.
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
    void drawFloor(FrameBuffer &s, const float *cx, const float *cy,
                   const float *r);

    float t = 0.0f;
    std::vector<float> mDist;     // sprite offset -> distance from centre
    std::vector<float> mAngle;    // sprite offset -> angle
    std::vector<uint8_t> mCanvas; // 256 x 64 brightness, so suns can overlap
  };

} /* namespace od */
