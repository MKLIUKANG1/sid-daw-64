#include "PianoRollWidget.h"
#include "Commands.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QUndoStack>
#include <algorithm>

PianoRollWidget::PianoRollWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    updateMinimumSize();
}

void PianoRollWidget::updateMinimumSize() {
    int pitches = highestPitch() - lowestPitch() + 1;
    setMinimumHeight(pitches * rowHeight());
    if (m_pattern && m_pattern->lengthTicks > 0)
        setMinimumWidth(keyWidth() + m_pattern->lengthTicks * m_zoom);
    else
        setMinimumWidth(keyWidth() + 64 * m_zoom);
}

void PianoRollWidget::setPattern(Pattern *pat) {
    m_pattern = pat;
    updateMinimumSize();
    update();
}

void PianoRollWidget::setZoom(int z) {
    m_zoom = std::clamp(z, 4, 48);
    updateMinimumSize();
    update();
}

int PianoRollWidget::yToPitch(int y) const {
    return highestPitch() - (y / rowHeight());
}
int PianoRollWidget::pitchToY(int pitch) const {
    return (highestPitch() - pitch) * rowHeight();
}
int PianoRollWidget::xToTick(int x) const {
    int gx = x - keyWidth();
    if (gx < 0) return -1;
    return gx / tickWidth();
}
int PianoRollWidget::tickToX(int tick) const {
    return keyWidth() + tick * tickWidth();
}

void PianoRollWidget::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        int delta = e->angleDelta().y() > 0 ? 2 : -2;
        setZoom(m_zoom + delta);
        e->accept();
    } else {
        QWidget::wheelEvent(e);
    }
}

void PianoRollWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 34));
    if (!m_pattern) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No pattern");
        return;
    }

    int kw = keyWidth();
    int rh = rowHeight();
    int tw = tickWidth();
    int top = 0;
    int bottom = (highestPitch() - lowestPitch() + 1) * rh;

    for (int pitch = lowestPitch(); pitch <= highestPitch(); ++pitch) {
        int y = pitchToY(pitch);
        QColor c = (pitch % 12 == 0) ? QColor(46, 46, 52) : QColor(36, 36, 40);
        p.fillRect(kw, y, width() - kw, rh, c);
    }

    p.fillRect(0, 0, kw, bottom, QColor(20, 20, 22));
    for (int pitch = lowestPitch(); pitch <= highestPitch(); ++pitch) {
        int y = pitchToY(pitch);
        int pc = pitch % 12;
        bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        p.fillRect(0, y, kw, rh, black ? QColor(26, 26, 30) : QColor(205, 205, 210));
        p.setPen(QColor(0, 0, 0, 80));
        p.drawLine(0, y, kw, y);
        if (pc == 0) {
            p.setPen(Qt::black);
            p.drawText(3, y + rh - 2, QString("C%1").arg(pitch / 12 - 1));
        }
    }
    p.setPen(QColor(10, 10, 10));
    p.drawLine(kw - 1, top, kw - 1, bottom);

    for (int t = 0; t <= m_pattern->lengthTicks; ++t) {
        int x = tickToX(t);
        if (t % 16 == 0)      p.setPen(QColor(110, 110, 125));
        else if (t % 4 == 0)  p.setPen(QColor(70, 70, 78));
        else                  p.setPen(QColor(48, 48, 54));
        p.drawLine(x, top, x, bottom);
    }

    for (const auto &n : m_pattern->notes) {
        int x = tickToX(n.tick);
        int y = pitchToY(n.pitch);
        int w = qMax(2, n.length * tw);
        int h = rh - 1;

        QColor col;
        switch (n.voice) {
            case 0: col = QColor( 80, 160, 255); break;
            case 1: col = QColor(120, 220, 120); break;
            default: col = QColor(240, 160,  80); break;
        }
        if (n.voice == m_currentVoice) col = col.lighter(115);

        QRect r(x, y, w, h);
        p.setBrush(col);
        p.setPen(col.darker(160));
        p.drawRoundedRect(r.adjusted(0, 1, -1, -1), 3, 3);
    }
}

PianoRollWidget::Hit PianoRollWidget::hitTest(const QPoint &pos) const {
    Hit h;
    if (!m_pattern) return h;
    for (int i = m_pattern->notes.size() - 1; i >= 0; --i) {
        const auto &n = m_pattern->notes[i];
        int x = tickToX(n.tick);
        int y = pitchToY(n.pitch);
        int w = qMax(2, n.length * tickWidth());
        int hgt = rowHeight() - 1;
        QRect r(x, y, w, hgt);
        if (r.contains(pos)) {
            h.index = i;
            h.onRightEdge = (pos.x() > x + w - 6);
            return h;
        }
    }
    return h;
}

void PianoRollWidget::mousePressEvent(QMouseEvent *e) {
    if (!m_pattern) return;
    if (e->pos().x() < keyWidth()) return;

    Hit h = hitTest(e->pos());
    if (e->button() == Qt::RightButton) {
        if (h.index >= 0) {
            if (m_undoStack)
                m_undoStack->push(new RemoveNoteCommand(m_pattern, h.index));
            else
                m_pattern->notes.removeAt(h.index);
            update();
            emit patternChanged();
        }
        return;
    }
    if (e->button() != Qt::LeftButton) return;

    if (h.index >= 0) {
        m_dragIndex  = h.index;
        m_dragResize = h.onRightEdge;
        m_dragging   = true;
        m_dragStart  = e->pos();
        const auto &n = m_pattern->notes[h.index];
        m_origTick   = n.tick;
        m_origLength = n.length;
        m_origPitch  = n.pitch;
        m_origNote   = n;
        m_dragChanged = false;
    } else {
        int tick  = xToTick(e->pos().x());
        int pitch = yToPitch(e->pos().y());
        if (tick < 0 || pitch < lowestPitch() || pitch > highestPitch()) return;
        Note n;
        n.tick    = tick;
        n.pitch   = pitch;
        n.length  = 4;
        n.velocity = 12;
        n.voice   = m_currentVoice;
        if (m_undoStack)
            m_undoStack->push(new AddNoteCommand(m_pattern, n));
        else
            m_pattern->notes.append(n);
        update();
        emit patternChanged();
    }
}

void PianoRollWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging || !m_pattern || m_dragIndex < 0) return;
    QPoint delta = e->pos() - m_dragStart;
    auto &n = m_pattern->notes[m_dragIndex];

    if (m_dragResize) {
        int newLen = m_origLength + delta.x() / tickWidth();
        n.length = qMax(1, newLen);
    } else {
        int tickDelta  =  delta.x() / tickWidth();
        int pitchDelta = -delta.y() / rowHeight();
        n.tick  = qMax(0, m_origTick + tickDelta);
        n.pitch = qBound(lowestPitch(), m_origPitch + pitchDelta, highestPitch());
    }
    m_dragChanged = true;
    update();
    emit patternChanged();
}

void PianoRollWidget::mouseReleaseEvent(QMouseEvent *) {
    if (m_dragging && m_dragChanged && m_pattern &&
        m_dragIndex >= 0 && m_dragIndex < m_pattern->notes.size()) {
        Note newN = m_pattern->notes[m_dragIndex];
        if (newN.tick   != m_origNote.tick   ||
            newN.pitch  != m_origNote.pitch  ||
            newN.length != m_origNote.length) {
            if (m_undoStack) {
                // Откатываем прямое изменение модели и пушим команду —
                // тогда redo() установит newN, undo() — m_origNote.
                m_pattern->notes[m_dragIndex] = m_origNote;
                m_undoStack->push(new ModifyNoteCommand(
                    m_pattern, m_dragIndex, m_origNote, newN));
            }
            emit patternChanged();
        }
    }
    m_dragging   = false;
    m_dragIndex  = -1;
    m_dragResize = false;
    m_dragChanged = false;
}
