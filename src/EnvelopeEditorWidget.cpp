#include "EnvelopeEditorWidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <algorithm>

EnvelopeEditorWidget::EnvelopeEditorWidget(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

void EnvelopeEditorWidget::setEnvelope(QVector<int> *env, int maxValue) {
    m_envelope = env;
    m_maxValue = std::max(1, maxValue);
    updateLength();
    update();
}

void EnvelopeEditorWidget::updateLength() {
    if (m_envelope && m_envelope->size() + 8 > m_length)
        m_length = m_envelope->size() + 8;
    if (m_length < 32) m_length = 32;
}

void EnvelopeEditorWidget::ensureEnvelopeSize(int index) {
    if (!m_envelope) return;
    while (m_envelope->size() <= index)
        m_envelope->append(m_envelope->isEmpty() ? 15 : m_envelope->last());
    if (m_length < m_envelope->size() + 8)
        m_length = m_envelope->size() + 8;
}

int EnvelopeEditorWidget::valueToY(int value) const {
    int h = height() - 8;
    if (h <= 0) return 4;
    return 4 + h - (value * h / m_maxValue);
}

int EnvelopeEditorWidget::yToValue(int y) const {
    int h = height() - 8;
    if (h <= 0) return 0;
    int v = (4 + h - y) * m_maxValue / h;
    return std::clamp(v, 0, m_maxValue);
}

int EnvelopeEditorWidget::xToIndex(int x) const {
    if (m_length <= 0) return 0;
    int w = width() - 8;
    if (w <= 0) return 0;
    return std::clamp((x - 4) * m_length / w, 0, m_length - 1);
}

int EnvelopeEditorWidget::indexToX(int index) const {
    if (m_length <= 0) return 4;
    int w = width() - 8;
    return 4 + (index * w / m_length);
}

void EnvelopeEditorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 34));

    // Сетка
    p.setPen(QColor(50, 50, 56));
    for (int i = 0; i <= 4; ++i) {
        int y = 4 + i * (height() - 8) / 4;
        p.drawLine(4, y, width() - 4, y);
    }
    int step = std::max(1, m_length / 8);
    for (int i = 0; i <= m_length; i += step) {
        int x = indexToX(i);
        p.drawLine(x, 4, x, height() - 4);
    }

    if (!m_envelope || m_envelope->isEmpty()) {
        p.setPen(QColor(140, 140, 150));
        p.drawText(rect().adjusted(4, 4, -4, -4),
                   Qt::AlignCenter,
                   "No envelope\n(click to draw)");
        return;
    }

    // График
    QPolygon poly;
    int n = std::min(m_length, int(m_envelope->size()));
    for (int i = 0; i < n; ++i)
        poly << QPoint(indexToX(i), valueToY((*m_envelope)[i]));

    p.setPen(QPen(QColor(100, 200, 255), 2));
    p.drawPolyline(poly);

    // Точки
    p.setBrush(QColor(100, 200, 255));
    for (int i = 0; i < n; ++i)
        p.drawEllipse(QPoint(indexToX(i), valueToY((*m_envelope)[i])), 2, 2);
}

void EnvelopeEditorWidget::mousePressEvent(QMouseEvent *e) {
    if (!m_envelope || e->button() != Qt::LeftButton) return;
    m_drawing = true;
    emit envelopeChangeStarted();
    int idx = xToIndex(e->pos().x());
    ensureEnvelopeSize(idx);
    (*m_envelope)[idx] = yToValue(e->pos().y());
    update();
    emit envelopeChanged();
}

void EnvelopeEditorWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_drawing || !m_envelope) return;
    int idx = xToIndex(e->pos().x());
    ensureEnvelopeSize(idx);
    (*m_envelope)[idx] = yToValue(e->pos().y());
    update();
    emit envelopeChanged();
}

void EnvelopeEditorWidget::mouseReleaseEvent(QMouseEvent *) {
    if (m_drawing) {
        m_drawing = false;
        emit envelopeChangeFinished();
    }
}
