#include "app.h"
#include "mainwindow.h"
#include <QDebug>
#include <string>
#include <cstring>

void Application::init()
{
    setApplicationName(tr("Early reflections"));

    MainWindow* window = new MainWindow;
    window->setWindowTitle(applicationDisplayName());
    window->show();

    ///
    jack_client_t* client = jack_client_open(applicationName().toStdString().c_str(), JackNoStartServer, nullptr);
    if (!client)
        qWarning() << "Cannot open jack client";
    else {
        client_.reset(client);
        for (int channel = 0; channel < 2; ++channel) {
            std::string inName = "input-" + std::to_string(channel + 1);
            std::string outName = "output-" + std::to_string(channel + 1);
            inPort_[channel] = jack_port_register(client, inName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
            outPort_[channel] = jack_port_register(client, outName.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        }

        float sampleRate = jack_get_sample_rate(client);
        for (int channel = 0; channel < 2; ++channel)
            erProc_[channel].init(sampleRate);

        jack_set_process_callback(client, &processAudio, this);
        if (jack_activate(client) != 0)
            qWarning() << "Cannot activate jack client";
    }
}

void Application::setStereoER(const double* leftPositions, const double* rightPositions, const double* leftGains, const double* rightGains, unsigned numLeftTaps, unsigned numRightTaps, double positionScale)
{
    std::lock_guard<std::mutex> lock(processLock_);
    erProc_[0].setER(leftPositions, leftGains, numLeftTaps, positionScale);
    erProc_[1].setER(rightPositions, rightGains, numRightTaps, positionScale);
}

int Application::processAudio(jack_nframes_t nframes, void* userData)
{
    Application* self = reinterpret_cast<Application*>(userData);

    const float* inputs[2];
    float* outputs[2];

    for (int channel = 0; channel < 2; ++channel) {
        inputs[channel] = (const float*)jack_port_get_buffer(self->inPort_[channel], nframes);
        outputs[channel] = (float*)jack_port_get_buffer(self->outPort_[channel], nframes);
    }

    std::unique_lock<std::mutex> lock(self->processLock_, std::try_to_lock);
    if (!lock.owns_lock()) {
        for (int channel = 0; channel < 2; ++channel)
            std::memset(outputs[channel], 0, nframes * sizeof(float));
        return 0;
    }

    for (int channel = 0; channel < 2; ++channel)
        self->erProc_[channel].process(inputs[channel], outputs[channel], nframes);

    return 0;
}

///
int main(int argc, char* argv[])
{
    Application app(argc, argv);

    app.init();

    return app.exec();
}
