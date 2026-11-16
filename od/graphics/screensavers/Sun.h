#pragma once

#include <od/graphics/ScreenSaver.h>
#include <vector>

namespace od
{

  // A small sun that slowly drifts across the main display along a
  // figure-of-eight path, with softly rotating rays and a gentle "breathing"
  // radius. The sub display shows a dotted horizon with the patch of light
  // the sun casts on the floor. Everything keeps moving to avoid burn-in.
  class Sun : public ScreenSaver
  {
  public:
    Sun();
    virtual ~Sun();

    virtual void reset();
    virtual void draw(FrameBuffer &mainFrameBuffer,
                      FrameBuffer &subFrameBuffer);

  private:
    float t = 0.0f;
    std::vector<float> mDist;  // distance of each sprite offset from centre
    std::vector<float> mAngle; // angle of each sprite offset
  };

} /* namespace od */
