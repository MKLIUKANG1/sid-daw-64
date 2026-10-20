#include "WaveformPreview.h"
#include <QPainter>

WaveformPreview::WaveformPreview(QWidget *parent) : QWidget(parent) {
    setFixedSize(64, 32);
}

void WaveformPreview::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(20, 20, 24));
    p.setPen(QColor(100, 200, 255));

    int w = width() - 4;
    int h = height() - 4;
    QPolygon poly;

    switch (m_waveform) {
        case 0: // Triangle
            poly << QPoint(2, 2 + h)
                 << QPoint(2 + w / 2, 2)
                 << QPoint(2 + w, 2 + h);
            break;
        case 1: // Sawtooth
            for (int x = 0; x <= w; ++x) {
                double t = double(x) / w;
                poly << QPoint(2 + x, 2 + h - int(t * h));
            }
            break;
        case 2: // Pulse 50%
            poly << QPoint(2, 2 + h) << QPoint(2, 2)
                 << QPoint(2 + w / 2, 2) << QPoint(2 + w / 2, 2 + h)
                 << QPoint(2 + w, 2 + h);
            break;
        case 3: // Noise
            for (int x = 0; x <= w; x += 3) {
                int y = 2 + (rand() % (h + 1));
                poly << QPoint(2 + x, y);
            }
            break;
    }
    p.drawPolyline(poly);

    p.setPen(QColor(60, 60, 70));
    p.drawRect(rect().adjusted(0, 0, -1, -1));
}
