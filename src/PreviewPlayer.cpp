#include "PreviewPlayer.h"
#include "SidEngine.h"
#include <QAudioSink>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QBuffer>
#include <QTimer>
#include <cmath>
#include <algorithm>

PreviewPlayer::PreviewPlayer(QObject *parent) : QObject(parent) {
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice dev = QMediaDevices::defaultAudioOutput();
    if (!dev.isFormatSupported(fmt)) {
        fmt = dev.preferredFormat();
        fmt.setSampleRate(44100);
        fmt.setChannelCount(1);
        fmt.setSampleFormat(QAudioFormat::Int16);
    }
    m_sink = new QAudioSink(dev, fmt, this);
    m_sink->setBufferSize(8192);
    m_fmt = new QAudioFormat(fmt);
}

PreviewPlayer::~PreviewPlayer() {
    if (m_sink) m_sink->stop();
    delete m_fmt;
}

QByteArray PreviewPlayer::renderPcm(const VoiceParams &vp, int midiPitch,
                                     int durationMs) {
    const int sr = 44100;
    const int totalSamples = sr * durationMs / 1000;

    SidEngine sid;
    sid.setSampleRate(sr);
    sid.reset();

    int base = 0;   // voice 0
    double f = 440.0 * std::pow(2.0, (midiPitch - 69) / 12.0);
    uint32_t freq = uint32_t(std::round(f * 16777216.0 / sid.clockFrequency())) & 0xFFFF;

    sid.write(base + 0, freq & 0xFF);
    sid.write(base + 1, (freq >> 8) & 0xFF);
    sid.write(base + 2, vp.pulseWidth & 0xFF);
    sid.write(base + 3, (vp.pulseWidth >> 8) & 0x0F);

    int sus = std::clamp(int(std::round(vp.sustain * vp.volume)), 0, 15);
    sid.write(base + 5, ((vp.attack & 0x0F) << 4) | (vp.decay & 0x0F));
    sid.write(base + 6, ((sus & 0x0F) << 4) | (vp.release & 0x0F));

    uint8_t ctrl = 0x01;
    switch (vp.waveform) {
        case 0: ctrl |= 0x10; break;
        case 1: ctrl |= 0x20; break;
        case 2: ctrl |= 0x40; break;
        case 3: ctrl |= 0x80; break;
    }
    if (vp.sync)    ctrl |= 0x02;
    if (vp.ringMod) ctrl |= 0x04;
    sid.write(base + 4, ctrl);

    sid.setFilterEnabled(true);
    sid.setFilterCutoff(1024);
    sid.setFilterResonance(8);
    sid.setFilterMode(0);

    QByteArray pcm(totalSamples * 2, 0);
    sid.clock(totalSamples, reinterpret_cast<int16_t*>(pcm.data()));
    return pcm;
}

void PreviewPlayer::playNote(const VoiceParams &vp, int midiPitch,
                              int durationMs) {
    if (!m_sink) return;
    if (m_sink->state() != QAudio::StoppedState)
        m_sink->stop();

    QByteArray pcm = renderPcm(vp, midiPitch, durationMs);

    // QBuffer не владеет QByteArray — копируем во владение
    auto *buf = new QBuffer(this);
    buf->setData(pcm);
    buf->open(QIODevice::ReadOnly);

    if (m_buffer && m_buffer != buf) {
        m_buffer->close();
        m_buffer->deleteLater();
    }
    m_buffer = buf;

    m_sink->start(buf);
}
