#include "DrumEditorWidget.h"
#include "DrumPresets.h"
#include "Commands.h"
#include <QPainter>
#include <QMouseEvent>
#include <QUndoStack>
#include <algorithm>

DrumEditorWidget::DrumEditorWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    updateMinimumSize();
}

void DrumEditorWidget::setPattern(Pattern *pat) {
    m_pattern = pat;
    updateMinimumSize();
    update();
}

int DrumEditorWidget::rowCount() const {
    if (!m_project) return 0;
    return m_project->drumKit.entries.size();
}
int DrumEditorWidget::rowPitch(int row) const {
    if (!m_project || row < 0 || row >= rowCount()) return 36;
    return m_project->drumKit.entries[row].midiPitch;
}
int DrumEditorWidget::rowInstrument(int row) const {
    if (!m_project || row < 0 || row >= rowCount()) return -1;
    return m_project->drumKit.entries[row].instrumentIndex;
}

int DrumEditorWidget::yToRow(int y) const { return y / rowHeight(); }
int DrumEditorWidget::xToTick(int x) const {
    int gx = x - labelWidth();
    if (gx < 0) return -1;
    return gx / tickWidth();
}
int DrumEditorWidget::tickToX(int t) const { return labelWidth() + t * tickWidth(); }
int DrumEditorWidget::rowToY(int r) const { return r * rowHeight(); }

void DrumEditorWidget::updateMinimumSize() {
    setMinimumHeight(std::max(120, rowCount() * rowHeight()));
    int ticks = m_pattern ? std::max(16, m_pattern->lengthTicks) : 64;
    setMinimumWidth(labelWidth() + ticks * tickWidth());
}

int DrumEditorWidget::findNoteAt(int row, int tick) const {
    if (!m_pattern) return -1;
    int targetPitch = rowPitch(row);
    for (int i = 0; i < m_pattern->notes.size(); ++i) {
        const Note &n = m_pattern->notes[i];
        if (n.pitch == targetPitch && n.tick == tick)
            return i;
    }
    return -1;
}

void DrumEditorWidget::ensureInstrumentForRow(int row) {
    if (!m_project || row < 0 || row >= rowCount()) return;
    DrumSlot &slot = m_project->drumKit.entries[row];
    if (slot.instrumentIndex >= 0 &&
        slot.instrumentIndex < (int)m_project->instruments.size())
        return;
    Instrument instr = DrumPresets::instrumentForPitch(slot.midiPitch);
    instr.name = slot.name;
    if (m_undoStack)
        m_undoStack->push(new AddInstrumentCommand(m_project, instr));
    else
        m_project->instruments.push_back(std::make_unique<Instrument>(instr));
    slot.instrumentIndex = int(m_project->instruments.size()) - 1;
}

void DrumEditorWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 28));

    int rows = rowCount();
    int ticks = m_pattern ? m_pattern->lengthTicks : 64;
    int rh = rowHeight(), tw = tickWidth(), lw = labelWidth();

    // Left labels
    p.fillRect(0, 0, lw, rows * rh, QColor(20, 20, 24));
    p.setPen(QColor(200, 200, 210));
    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    for (int r = 0; r < rows; ++r) {
        int y = rowToY(r);
        p.setPen(QColor(16, 16, 20));
        p.drawLine(0, y + rh - 1, lw, y + rh - 1);
        p.setPen(QColor(200, 200, 210));
        QString text = m_project->drumKit.entries[r].name;
        if (text.isEmpty())
            text = DrumPresets::nameForPitch(rowPitch(r));
        p.drawText(QRect(6, y, lw - 8, rh),
                   Qt::AlignVCenter | Qt::AlignLeft, text);
    }

    // Grid
    for (int r = 0; r < rows; ++r) {
        int y = rowToY(r);
        for (int t = 0; t < ticks; ++t) {
            int x = tickToX(t);
            QColor cell = ((t / 4) % 4 == 0) ? QColor(38, 38, 44) : QColor(30, 30, 35);
            if (t % 4 == 0) cell = cell.lighter(115);
            p.fillRect(x, y, tw, rh, cell);
            p.setPen(QColor(20, 20, 24));
            p.drawRect(x, y, tw, rh);
        }
    }

    // Triggers
    if (!m_pattern) return;
    for (int r = 0; r < rows; ++r) {
        int targetPitch = rowPitch(r);
        int instrIdx = rowInstrument(r);
        QColor base;
        if (instrIdx >= 0 && m_project &&
            instrIdx < (int)m_project->instruments.size()) {
            static const QColor palette[8] = {
                QColor( 80, 160, 255), QColor(120, 220, 120),
                QColor(240, 160,  80), QColor(210, 120, 200),
                QColor( 90, 200, 200), QColor(255, 220, 100),
                QColor(200, 140, 255), QColor(150, 200, 130),
            };
            base = palette[instrIdx % 8];
        } else {
            base = QColor(240, 160, 80);
        }

        for (const auto &n : m_pattern->notes) {
            if (n.pitch != targetPitch) continue;
            int x = tickToX(n.tick);
            int y = rowToY(r);
            QRect r2(x + 2, y + 2, tw - 4, rh - 4);
            p.setBrush(base);
            p.setPen(base.darker(170));
            p.drawRoundedRect(r2, 3, 3);
        }
    }
}

void DrumEditorWidget::mousePressEvent(QMouseEvent *e) {
    if (!m_pattern || !m_project) return;

    if (e->button() == Qt::MiddleButton) {
        m_panning = true;
        m_panStart = e->pos();
        setCursor(Qt::ClosedHandCursor);
        e->accept();
        return;
    }
    if (e->pos().x() < labelWidth()) return;
    if (e->button() != Qt::LeftButton && e->button() != Qt::RightButton) return;

    int row = yToRow(e->pos().y());
    int tick = xToTick(e->pos().x());
    if (row < 0 || row >= rowCount() || tick < 0 || tick >= m_pattern->lengthTicks)
        return;

    int existing = findNoteAt(row, tick);

    if (e->button() == Qt::RightButton || (e->modifiers() & Qt::ControlModifier)) {
        if (existing >= 0) {
            if (m_undoStack)
                m_undoStack->push(new RemoveNoteCommand(m_pattern, existing));
            else
                m_pattern->notes.removeAt(existing);
            update();
            emit patternChanged();
        }
        return;
    }

    if (existing >= 0) {
        if (m_undoStack)
            m_undoStack->push(new RemoveNoteCommand(m_pattern, existing));
        else
            m_pattern->notes.removeAt(existing);
    } else {
        ensureInstrumentForRow(row);
        Note n;
        n.tick     = tick;
        n.pitch    = rowPitch(row);
        n.length   = 1;
        n.velocity = 12;
        n.voice    = m_currentVoice;
        n.instrumentIndex = rowInstrument(row);
        if (m_undoStack)
            m_undoStack->push(new AddNoteCommand(m_pattern, n));
        else
            m_pattern->notes.append(n);
    }
    update();
    emit patternChanged();
}

void DrumEditorWidget::mouseMoveEvent(QMouseEvent *e) {
    if (m_panning) {
        QPoint d = m_panStart - e->pos();
        emit panBy(d.x(), d.y());
        m_panStart = e->pos();
    }
}

void DrumEditorWidget::mouseReleaseEvent(QMouseEvent *e) {
    if (m_panning && e->button() == Qt::MiddleButton) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
    }
}
