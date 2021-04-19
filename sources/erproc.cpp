#include "erproc.h"
#include <cstring>
#include <cmath>

static const float kMaxHistoryDelay = 200e-3;

void ERproc::init(float sampleRate)
{
    sampleRate_ = sampleRate;
    historyIndex_ = 0;
    historyFrames_ = (unsigned)std::ceil(kMaxHistoryDelay * sampleRate);
    historyBuffer_.reset(new float[2 * historyFrames_]{});
}

void ERproc::clear()
{
    std::memset(historyBuffer_.get(), 0, (2 * historyFrames_) * sizeof(float));
}

void ERproc::setER(const double* positions, const double* gains, unsigned numTaps, double positionScale)
{
    numTaps_ = numTaps;

    framePositions_.reset(new unsigned[numTaps]);
    for (unsigned i = 0; i < numTaps; ++i)
        framePositions_[i] = (unsigned)(0.5 + positions[i] * positionScale * sampleRate_);

    gains_.reset(new float[numTaps]);
    for (unsigned i = 0; i < numTaps; ++i)
        gains_[i] = gains[i];

    clear();
}

void ERproc::process(const float* input, float* output, unsigned nframes)
{
    unsigned historyIndex = historyIndex_;
    unsigned historyFrames = historyFrames_;
    float* historyBuffer = historyBuffer_.get();

    unsigned numTaps = numTaps_;
    const unsigned* framePositions = framePositions_.get();
    const float* gains = gains_.get();

    for (unsigned i = 0; i < nframes; ++i) {
        historyBuffer[historyIndex] = historyBuffer[historyIndex + historyFrames] = input[i];

        float out = 0.0f;
        for (unsigned j = 0; j < numTaps; ++j)
            out += gains[j] * historyBuffer[historyIndex + historyFrames - framePositions[j]];
        output[i] = out;

        historyIndex = (historyIndex + 1) % historyFrames;
    }

    historyIndex_ = historyIndex;
}
