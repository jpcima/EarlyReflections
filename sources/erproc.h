#pragma once
#include <memory>

class ERproc {
public:
    void init(float sampleRate);
    void clear();
    void setER(const double* positions, const double* gains, unsigned numTaps, double positionScale);
    void process(const float* input, float* output, unsigned nframes);

private:
    float sampleRate_ = 0;

    unsigned historyIndex_ = 0;
    unsigned historyFrames_ = 0;
    std::unique_ptr<float[]> historyBuffer_;

    unsigned numTaps_ = 0;
    std::unique_ptr<unsigned[]> framePositions_;
    std::unique_ptr<float[]> gains_;
};
