#pragma once

#include <od/graphics/ScreenSaver.h>

namespace od
{

  // The thirteen sound files recovered from a second-hand SD card, laid out
  // like a folder listing. A silent "playhead" walks through them in the
  // order they were found on the card, lighting each for the real length of
  // its recording. The sub display shows a scrolling waveform.
  class Wavs : public ScreenSaver
  {
  public:
    Wavs();
    virtual ~Wavs();

    virtual void reset();
    virtual void draw(FrameBuffer &mainFrameBuffer,
                      FrameBuffer &subFrameBuffer);

  private:
    float mTime = 0.0f;   // seconds into the current file
    int mCurrent = 0;     // index of the file being "played"
    float mPause = 0.0f;  // silence between passes
    float mDrift = 0.0f;  // slow drift of the whole layout (avoids burn-in)
    float mScroll = 0.0f; // waveform scroll position
  };

} /* namespace od */
