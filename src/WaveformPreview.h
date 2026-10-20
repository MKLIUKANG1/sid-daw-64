#pragma once
#include <QWidget>

class WaveformPreview : public QWidget {
    Q_OBJECT
public:
    explicit WaveformPreview(QWidget *parent = nullptr);
    void setWaveform(int w) { m_waveform = w; update(); }
protected:
    void paintEvent(QPaintEvent *) override;
private:
    int m_waveform = 0;
};
