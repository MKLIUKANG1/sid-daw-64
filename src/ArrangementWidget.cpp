#include "ArrangementWidget.h"
#include "Commands.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QUndoStack>
#include <QPolygon>
#include <QFont>
#include <algorithm>

ArrangementWidget::ArrangementWidget(QWidget *parent) : QWidget(parent) {
    setMouseTracking(true);
    updateMinimumSize();
}

void ArrangementWidget::updateMinimumSize() {
    setMinimumHeight(rulerHeight() + 4 * trackHeight() + 4);
    if (m_project)
        setMinimumWidth(labelWidth() + m_project->totalTicks * m_tickWidth);
    else
        setMinimumWidth(labelWidth() + 256 * m_tickWidth);
}

void ArrangementWidget::setProject(Project *p) {
    m_project = p;
    updateMinimumSize();
    update();
}

void ArrangementWidget::setZoom(int z) {
    m_tickWidth = std::clamp(z, 1, 24);
    updateMinimumSize();
    update();
}

int ArrangementWidget::xToTick(int x) const { return (x - labelWidth()) / tickWidth(); }
int ArrangementWidget::tickToX(int t) const { return labelWidth() + t * tickWidth(); }
int ArrangementWidget::yToTrack(int y) const { return (y - rulerHeight()) / trackHeight(); }
int ArrangementWidget::trackToY(int tr) const { return rulerHeight() + tr * trackHeight(); }

void ArrangementWidget::wheelEvent(QWheelEvent *e) {
    if (e->modifiers() & Qt::ControlModifier) {
        int delta = e->angleDelta().y() > 0 ? 1 : -1;
        setZoom(m_tickWidth + delta);
        e->accept();
    } else {
        QWidget::wheelEvent(e);
    }
}

void ArrangementWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.fillRect(rect(), QColor(24, 24, 28));
    if (!m_project) return;

    const int lw = labelWidth();
    const int rh = rulerHeight();
    const int th = trackHeight();
    const int tw = tickWidth();
    const int tpb = ticksPerBar();
    const int totalH = rh + m_project->trackCount * th;

    p.fillRect(0, 0, width(), rh, QColor(36, 36, 42));
    p.setPen(QColor(16, 16, 20));
    p.drawLine(0, rh - 1, width(), rh - 1);

    QFont f = p.font();
    f.setPointSize(8);
    p.setFont(f);
    int barStep = 1;
    while (barStep * tpb * tw < 40) barStep++;
    for (int t = 0; t <= m_project->totalTicks; t += tpb * barStep) {
        int x = tickToX(t);
        if (x < lw) continue;
        p.setPen(QColor(70, 70, 80));
        p.drawLine(x, 0, x, rh);
        p.setPen(QColor(160, 160, 170));
        p.drawText(x + 4, rh - 7, QString("Bar %1").arg(t / tpb + 1));
    }

    for (int tr = 0; tr < m_project->trackCount; ++tr) {
        int y = trackToY(tr);
        QColor bg = (tr % 2 == 0) ? QColor(28, 28, 33) : QColor(25, 25, 30);
        p.fillRect(lw, y, width() - lw, th, bg);
        p.fillRect(0, y, lw, th, QColor(20, 20, 24));
        p.setPen(QColor(200, 200, 210));
        p.drawText(10, y + th / 2 + 4, QString("Track %1").arg(tr + 1));
        p.setPen(QColor(14, 14, 18));
        p.drawLine(0, y + th - 1, width(), y + th - 1);
    }

    for (int t = 0; t <= m_project->totalTicks; t += snapTicks()) {
        int x = tickToX(t);
        if (x < lw) continue;
        p.setPen(t % tpb == 0 ? QColor(64, 64, 74) : QColor(40, 40, 46));
        p.drawLine(x, rh, x, totalH);
    }
    p.setPen(QColor(14, 14, 18));
    p.drawLine(lw - 1, 0, lw - 1, totalH);

    for (const auto &c : m_project->clips) {
        int x = tickToX(c.startTick);
        int w = c.lengthTicks * tw;
        int y = trackToY(c.track) + 3;
        int h = th - 6;

        QColor base;
        switch (c.patternIndex % 5) {
            case 0: base = QColor(100, 150, 230); break;
            case 1: base = QColor(140, 200, 120); break;
            case 2: base = QColor(230, 170,  90); break;
            case 3: base = QColor(210, 120, 200); break;
            default:base = QColor( 90, 200, 200); break;
        }
        if (c.patternIndex == m_activePattern) base = base.lighter(112);
        QRect r(x, y, qMax(4, w), h);
        p.setBrush(base);
        p.setPen(base.darker(170));
        p.drawRoundedRect(r, 4, 4);

        p.setPen(Qt::white);
        QString name = (c.patternIndex < (int)m_project->patterns.size())
                         ? m_project->patterns[c.patternIndex]->name
                         : QString("?");
        p.drawText(r.adjusted(6, 3, -4, -2), Qt::AlignLeft | Qt::AlignTop, name);
    }

    int px = tickToX(int(m_playheadTick));
    p.setPen(QPen(QColor(255, 90, 90), 2));
    p.drawLine(px, 0, px, totalH);
    QPolygon tri;
    tri << QPoint(px - 5, 0) << QPoint(px + 5, 0) << QPoint(px, 7);
    p.setBrush(QColor(255, 90, 90));
    p.setPen(Qt::NoPen);
    p.drawPolygon(tri);
}

ArrangementWidget::Hit ArrangementWidget::hitTest(const QPoint &pos) const {
    Hit h;
    if (!m_project) return h;
    for (int i = m_project->clips.size() - 1; i >= 0; --i) {
        const auto &c = m_project->clips[i];
        int x = tickToX(c.startTick);
        int y = trackToY(c.track) + 3;
        int w = c.lengthTicks * tickWidth();
        int hgt = trackHeight() - 6;
        QRect r(x, y, w, hgt);
        if (r.contains(pos)) {
            h.index = i;
            h.onRightEdge = (pos.x() > x + w - 6);
            return h;
        }
    }
    return h;
}

void ArrangementWidget::mousePressEvent(QMouseEvent *e) {
    if (!m_project) return;
    const int lw = labelWidth();

    if (e->pos().y() < rulerHeight()) {
        if (e->pos().x() >= lw) {
            int t = qMax(0, xToTick(e->pos().x()));
            m_playheadTick = double(t);
            update();
            emit seekRequested(m_playheadTick);
        }
        return;
    }
    if (e->pos().x() < lw) return;

    Hit h = hitTest(e->pos());
    if (e->button() == Qt::RightButton) {
        if (h.index >= 0) {
            if (m_undoStack)
                m_undoStack->push(new RemoveClipCommand(m_project, h.index));
            else
                m_project->clips.removeAt(h.index);
            update();
            emit projectChanged();
        }
        return;
    }
    if (e->button() != Qt::LeftButton) return;

    if (h.index >= 0) {
        m_dragIndex  = h.index;
        m_dragResize = h.onRightEdge;
        m_dragging   = true;
        m_dragStart  = e->pos();
        const auto &c = m_project->clips[h.index];
        m_origStart  = c.startTick;
        m_origLength = c.lengthTicks;
        m_origTrack  = c.track;
        m_origClip   = c;
        m_dragChanged = false;
    } else {
        int t  = xToTick(e->pos().x());
        int tr = yToTrack(e->pos().y());
        if (t < 0 || tr < 0 || tr >= m_project->trackCount) return;
        t = (t / snapTicks()) * snapTicks();

        Clip c;
        c.patternIndex = m_activePattern;
        c.track        = tr;
        c.startTick    = t;
        c.lengthTicks  = (m_activePattern < (int)m_project->patterns.size())
                           ? m_project->patterns[m_activePattern]->lengthTicks
                           : 16;
        if (m_undoStack)
            m_undoStack->push(new AddClipCommand(m_project, c));
        else
            m_project->clips.append(c);
        update();
        emit projectChanged();
    }
}

void ArrangementWidget::mouseMoveEvent(QMouseEvent *e) {
    if (!m_dragging || !m_project || m_dragIndex < 0) return;
    auto &c = m_project->clips[m_dragIndex];
    int dx = e->pos().x() - m_dragStart.x();
    int dticks = dx / tickWidth();

    if (m_dragResize) {
        int newLen = m_origLength + dticks;
        newLen = (newLen / snapTicks()) * snapTicks();
        c.lengthTicks = qMax(snapTicks(), newLen);
    } else {
        int newStart = m_origStart + dticks;
        newStart = (newStart / snapTicks()) * snapTicks();
        c.startTick = qMax(0, newStart);
        int dy = e->pos().y() - m_dragStart.y();
        int newTrack = m_origTrack + (dy / trackHeight());
        c.track = qBound(0, newTrack, m_project->trackCount - 1);
    }
    m_dragChanged = true;
    update();
    emit projectChanged();
}

void ArrangementWidget::mouseReleaseEvent(QMouseEvent *) {
    if (m_dragging && m_dragChanged && m_project &&
        m_dragIndex >= 0 && m_dragIndex < m_project->clips.size()) {
        Clip newC = m_project->clips[m_dragIndex];
        if (newC.startTick   != m_origClip.startTick   ||
            newC.lengthTicks != m_origClip.lengthTicks ||
            newC.track       != m_origClip.track) {
            if (m_undoStack) {
                m_project->clips[m_dragIndex] = m_origClip;
                m_undoStack->push(new ModifyClipCommand(
                    m_project, m_dragIndex, m_origClip, newC));
            }
            emit projectChanged();
        }
    }
    m_dragging   = false;
    m_dragIndex  = -1;
    m_dragResize = false;
    m_dragChanged = false;
}
