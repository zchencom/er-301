#pragma once

#include <od/graphics/ScreenSaver.h>

namespace od
{

  // The thirteen sound files recovered from a second-hand SD card, laid out
  // like a folder listing. A silent "playhead" walks through them in the
  // order they were found on the card, lingering on each for the real length
  // of its recording. The sub display shows the file being "played", where
  // on the card it was found, and a map of the card with all thirteen marks.
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
    float mFade[13];      // afterglow of files already played
  };

} /* namespace od */
