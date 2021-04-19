#pragma once
#include "erproc.h"
#include <QApplication>
#include <jack/jack.h>
#include <mutex>
#include <memory>

class Application : public QApplication {
public:
    using QApplication::QApplication;
    void init();

    void setStereoER(const double* leftPositions, const double* rightPositions, const double* leftGains, const double* rightGains, unsigned numLeftTaps, unsigned numRightTaps, double positionScale);
    void setBypassed(bool bypassed);

private:
    static int processAudio(jack_nframes_t nframes, void* userData);

private:
    struct jack_client_delete {
        void operator()(jack_client_t* x) { jack_client_close(x); }
    };

    ERproc erProc_[2];
    bool bypassed_ = false;
    std::mutex processLock_;
    jack_port_t* inPort_[2] = {};
    jack_port_t* outPort_[2] = {};
    std::unique_ptr<jack_client_t, jack_client_delete> client_;
};
