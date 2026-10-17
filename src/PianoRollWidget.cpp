#include "PianoRollWidget.h"
#include "Commands.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QUndoStack>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <algorithm>

PianoRollWidget::PianoRollWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
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
    m_selected.clear();
    updateMinimumSize();
    update();
}

void PianoRollWidget::setZoom(int z) {
    m_zoom = std::clamp(z, 4, 48);
    updateMinimumSize();
    update();
}

void PianoRollWidget::clearSelection() { m_selected.clear(); update(); }

int PianoRollWidget::yToPitch(int y) const { return highestPitch() - (y / rowHeight()); }
int PianoRollWidget::pitchToY(int pitch) const { return (highestPitch() - pitch) * rowHeight(); }
int PianoRollWidget::xToTick(int x) const {
    int gx = x - keyWidth();
    if (gx < 0) return -1;
    return gx / tickWidth();
}
int PianoRollWidget::tickToX(int tick) const { return keyWidth() + tick * tickWidth(); }

QColor PianoRollWidget::colorForNote(const Note &n) const {
    if (n.instrumentIndex >= 0 && m_project &&
        n.instrumentIndex < (int)m_project->instruments.size()) {
        static const QColor palette[8] = {
            QColor( 80, 160, 255), QColor(120, 220, 120),
            QColor(240, 160,  80), QColor(210, 120, 200),
            QColor( 90, 200, 200), QColor(255, 220, 100),
            QColor(200, 140, 255), QColor(150, 200, 130),
        };
        return palette[n.instrumentIndex % 8];
    }
    switch (n.voice) {
        case 0: return QColor( 80, 160, 255);
        case 1: return QColor(120, 220, 120);
        default:return QColor(240, 160,  80);
    }
}

void PianoRollWidget::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        int delta = e->angleDelta().y() > 0 ? 2 : -2;
        setZoom(m_zoom + delta);
        e->accept();
    } else QWidget::wheelEvent(e);
}

void PianoRollWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 30, 34));
    if (!m_pattern) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "No pattern");
        return;
    }
    int kw = keyWidth(), rh = rowHeight(), tw = tickWidth();
    int bottom = (highestPitch() - lowestPitch() + 1) * rh;

    for (int pitch = lowestPitch(); pitch <= highestPitch(); ++pitch) {
        int y = pitchToY(pitch);
        p.fillRect(kw, y, width() - kw, rh,
                   (pitch % 12 == 0) ? QColor(46, 46, 52) : QColor(36, 36, 40));
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
    p.drawLine(kw - 1, 0, kw - 1, bottom);

    for (int t = 0; t <= m_pattern->lengthTicks; ++t) {
        int x = tickToX(t);
        if (t % 16 == 0)      p.setPen(QColor(110, 110, 125));
        else if (t % 4 == 0)  p.setPen(QColor(70, 70, 78));
        else                  p.setPen(QColor(48, 48, 54));
        p.drawLine(x, 0, x, bottom);
    }

    for (int i = 0; i < m_pattern->notes.size(); ++i) {
        const auto &n = m_pattern->notes[i];
        int x = tickToX(n.tick), y = pitchToY(n.pitch);
        int w = qMax(2, n.length * tw), h = rh - 1;
        QColor col = colorForNote(n);
        if (n.voice == m_currentVoice) col = col.lighter(112);
        QRect r(x, y, w, h);
        p.setBrush(col);
        p.setPen(col.darker(160));
        p.drawRoundedRect(r.adjusted(0, 1, -1, -1), 3, 3);
        if (m_selected.contains(i)) {
            p.setPen(QPen(QColor(255, 255, 100), 2));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(r.adjusted(0, 1, -1, -1), 3, 3);
        }
    }

    if (m_rubberBanding) {
        QRect r = QRect(m_rubberStart, m_rubberEnd).normalized();
        p.setPen(QPen(QColor(100, 200, 255), 1, Qt::DashLine));
        p.setBrush(QColor(100, 200, 255, 40));
        p.drawRect(r);
    }
}

PianoRollWidget::Hit PianoRollWidget::hitTest(const QPoint &pos) const {
    Hit h;
    if (!m_pattern) return h;
    for (int i = m_pattern->notes.size() - 1; i >= 0; --i) {
        const auto &n = m_pattern->notes[i];
        int x = tickToX(n.tick), y = pitchToY(n.pitch);
        int w = qMax(2, n.length * tickWidth());
        QRect r(x, y, w, rowHeight() - 1);
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

    if (e->button() == Qt::MiddleButton) {
        Hit h = hitTest(e->pos());
        QVector<int> indices;
        if (h.index >= 0) {
            if (m_selected.contains(h.index)) {
                for (int i : m_selected) indices.append(i);
            } else {
                m_selected.clear();
                m_selected.insert(h.index);
                indices.append(h.index);
                update();
            }
            emit requestNoteDialog(indices);
            return;
        }
        if (!m_selected.isEmpty()) {
            for (int i : m_selected) indices.append(i);
            emit requestNoteDialog(indices);
            return;
        }
        m_panning  = true;
        m_panStart = e->pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }

    if (e->pos().x() < keyWidth()) return;

    bool ctrl  = (e->modifiers() & Qt::ControlModifier);
    bool shift = (e->modifiers() & Qt::ShiftModifier);
    Hit h = hitTest(e->pos());

    if (e->button() == Qt::RightButton) {
        if (h.index >= 0) {
            if (m_undoStack) m_undoStack->push(new RemoveNoteCommand(m_pattern, h.index));
            else m_pattern->notes.removeAt(h.index);
            m_selected.remove(h.index);
            QSet<int> ns;
            for (int idx : m_selected) ns.insert(idx > h.index ? idx - 1 : idx);
            m_selected = ns;
            update();
            emit patternChanged();
        }
        return;
    }

    if (e->button() != Qt::LeftButton) return;

    if (h.index >= 0) {
        if (ctrl) {
            if (m_selected.contains(h.index)) m_selected.remove(h.index);
            else m_selected.insert(h.index);
            update();
            return;
        }
        if (!m_selected.contains(h.index)) {
            m_selected.clear();
            m_selected.insert(h.index);
        }
        emit noteInstrumentChanged(m_pattern->notes[h.index].instrumentIndex);
        m_dragIndex = h.index;
        m_dragResize = h.onRightEdge;
        m_dragging = true;
        m_dragStart = e->pos();
        const auto &n = m_pattern->notes[h.index];
        m_origTick = n.tick; m_origLength = n.length; m_origPitch = n.pitch;
        m_origNote = n;
        m_dragChanged = false;
        m_dragIndices.clear(); m_dragOldNotes.clear();
        for (int i : m_selected) {
            m_dragIndices.append(i);
            m_dragOldNotes.append(m_pattern->notes[i]);
        }
        update();
        return;
    }

    if (shift) {
        m_rubberBanding = true;
        m_rubberStart = e->pos();
        m_rubberEnd = e->pos();
        update();
        return;
    }

    m_selected.clear();
    int tick = xToTick(e->pos().x());
    int pitch = yToPitch(e->pos().y());
    if (tick < 0 || pitch < lowestPitch() || pitch > highestPitch()) return;
    Note n;
    n.tick = tick; n.pitch = pitch; n.length = 4; n.velocity = 12;
    n.voice = m_currentVoice; n.instrumentIndex = m_currentInstrument;
    if (m_undoStack) m_undoStack->push(new AddNoteCommand(m_pattern, n));
    else m_pattern->notes.append(n);
    update();
    emit patternChanged();
}

void PianoRollWidget::mouseMoveEvent(QMouseEvent *e) {
    if (m_panning) {
        QPoint d = m_panStart - e->pos();
        emit panBy(d.x(), d.y());
        m_panStart = e->pos();
        return;
    }
    if (m_rubberBanding) { m_rubberEnd = e->pos(); update(); return; }
    if (!m_dragging || !m_pattern || m_dragIndex < 0) return;

    QPoint delta = e->pos() - m_dragStart;
    if (m_dragResize) {
        auto &n = m_pattern->notes[m_dragIndex];
        n.length = qMax(1, m_origLength + delta.x() / tickWidth());
    } else {
        int tD = delta.x() / tickWidth();
        int pD = -delta.y() / rowHeight();
        for (int k = 0; k < m_dragIndices.size(); ++k) {
            Note nn = m_dragOldNotes[k];
            nn.tick = qMax(0, m_dragOldNotes[k].tick + tD);
            nn.pitch = qBound(lowestPitch(),
                              m_dragOldNotes[k].pitch + pD, highestPitch());
            m_pattern->notes[m_dragIndices[k]] = nn;
        }
    }
    m_dragChanged = true;
    update();
    emit patternChanged();
}

void PianoRollWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (m_panning && e->button() == Qt::MiddleButton) {
        m_panning = false; setCursor(Qt::ArrowCursor); return;
    }
    if (m_rubberBanding) {
        m_rubberBanding = false;
        QRect rect = QRect(m_rubberStart, m_rubberEnd).normalized();
        m_selected.clear();
        for (int i = 0; i < m_pattern->notes.size(); ++i) {
            const auto &n = m_pattern->notes[i];
            int x = tickToX(n.tick), y = pitchToY(n.pitch);
            int w = qMax(2, n.length * tickWidth()), h = rowHeight() - 1;
            if (rect.intersects(QRect(x, y, w, h))) m_selected.insert(i);
        }
        update();
        return;
    }
    if (m_dragging && m_dragChanged && m_pattern) {
        if (m_dragResize && m_dragIndex >= 0) {
            Note newN = m_pattern->notes[m_dragIndex];
            if (newN.length != m_origNote.length && m_undoStack) {
                m_pattern->notes[m_dragIndex] = m_origNote;
                m_undoStack->push(new ModifyNoteCommand(
                    m_pattern, m_dragIndex, m_origNote, newN));
            }
            emit patternChanged();
        } else if (!m_dragResize && !m_dragIndices.isEmpty()) {
            QVector<Note> newNotes;
            bool any = false;
            for (int k = 0; k < m_dragIndices.size(); ++k) {
                Note nn = m_pattern->notes[m_dragIndices[k]];
                newNotes.append(nn);
                if (nn.tick != m_dragOldNotes[k].tick ||
                    nn.pitch != m_dragOldNotes[k].pitch) any = true;
            }
            if (any && m_undoStack) {
                for (int k = 0; k < m_dragIndices.size(); ++k)
                    m_pattern->notes[m_dragIndices[k]] = m_dragOldNotes[k];
                m_undoStack->push(new MoveNotesCommand(
                    m_pattern, m_dragIndices, m_dragOldNotes, newNotes));
            }
            emit patternChanged();
        }
    }
    m_dragging = false; m_dragIndex = -1; m_dragResize = false; m_dragChanged = false;
    m_dragIndices.clear(); m_dragOldNotes.clear();
}

void PianoRollWidget::commitKeyboardMove(int pD, int tD) {
    if (!m_pattern || m_selected.isEmpty()) return;
    for (int i : m_selected) {
        if (m_pattern->notes[i].pitch + pD < lowestPitch() ||
            m_pattern->notes[i].pitch + pD > highestPitch()) return;
        if (m_pattern->notes[i].tick + tD < 0) return;
    }
    QVector<int> idx;
    QVector<Note> oldN, newN;
    for (int i : m_selected) {
        idx.append(i);
        oldN.append(m_pattern->notes[i]);
        Note n = m_pattern->notes[i];
        n.pitch += pD; n.tick += tD;
        newN.append(n);
    }
    if (m_undoStack) m_undoStack->push(new MoveNotesCommand(m_pattern, idx, oldN, newN));
    else for (int k = 0; k < idx.size(); ++k) m_pattern->notes[idx[k]] = newN[k];
    update();
    emit patternChanged();
}

void PianoRollWidget::keyPressEvent(QKeyEvent *e) {
    if (!m_pattern) { QWidget::keyPressEvent(e); return; }
    bool shift = (e->modifiers() & Qt::ShiftModifier);
    bool ctrl  = (e->modifiers() & Qt::ControlModifier);

    if (ctrl && e->key() == Qt::Key_A) {
        m_selected.clear();
        for (int i = 0; i < m_pattern->notes.size(); ++i) m_selected.insert(i);
        update();
        return;
    }
    if (e->key() == Qt::Key_F) {
        if (!m_selected.isEmpty()) {
            QVector<int> idx;
            for (int i : m_selected) idx.append(i);
            emit requestNoteDialog(idx);
        }
        e->accept();
        return;
    }
    if (e->key() == Qt::Key_Escape) { clearSelection(); return; }

    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        if (!m_selected.isEmpty()) {
            QVector<int> idx;
            for (int i : m_selected) idx.append(i);
            if (m_undoStack) m_undoStack->push(new RemoveNotesCommand(m_pattern, idx));
            else for (int i = idx.size() - 1; i >= 0; --i) m_pattern->notes.removeAt(idx[i]);
            m_selected.clear();
            update();
            emit patternChanged();
        }
        return;
    }
    if (m_selected.isEmpty()) { QWidget::keyPressEvent(e); return; }

    int pStep = shift ? 12 : 1;
    int tStep = shift ? 4  : 1;
    switch (e->key()) {
        case Qt::Key_Up:    commitKeyboardMove( pStep, 0); break;
        case Qt::Key_Down:  commitKeyboardMove(-pStep, 0); break;
        case Qt::Key_Left:  commitKeyboardMove(0, -tStep);  break;
        case Qt::Key_Right: commitKeyboardMove(0,  tStep);  break;
        default: QWidget::keyPressEvent(e); return;
    }
    e->accept();
}

void PianoRollWidget::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("application/x-siddaw-instrument"))
        e->acceptProposedAction();
    else e->ignore();
}

void PianoRollWidget::dragMoveEvent(QDragMoveEvent *e) {
    if (!e->mimeData()->hasFormat("application/x-siddaw-instrument")) {
        e->ignore(); return;
    }
    Hit h = hitTest(e->position().toPoint());
    if (h.index >= 0) { e->setDropAction(Qt::CopyAction); e->accept(); }
    else e->ignore();
}

void PianoRollWidget::dropEvent(QDropEvent *e) {
    if (!e->mimeData()->hasFormat("application/x-siddaw-instrument")) {
        e->ignore(); return;
    }
    int idx = e->mimeData()->data("application/x-siddaw-instrument").toInt();
    Hit h = hitTest(e->position().toPoint());
    if (h.index < 0) { e->ignore(); return; }

    QVector<int> indices;
    if (m_selected.contains(h.index)) {
        for (int i : m_selected) indices.append(i);
    } else {
        indices.append(h.index);
    }
    emit instrumentDropped(indices, idx);
    e->acceptProposedAction();
}
