#pragma once
#include <QWidget>
#include "Model.h"

class QUndoStack;

class ArrangementWidget : public QWidget {
    Q_OBJECT
public:
    explicit ArrangementWidget(QWidget *parent = nullptr);

    void setProject(Project *p);
    void setActivePattern(int idx) { m_activePattern = idx; update(); }
    void setPlayheadTick(double t) { m_playheadTick = t; update(); }
    double playheadTick() const { return m_playheadTick; }
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }
    void setZoom(int pixelsPerTick);
    int  zoom() const { return m_tickWidth; }

signals:
    void addPatternRequested(int track, int startTick);
    void projectChanged();
    void seekRequested(double tick);
    void panBy(int dx, int dy);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    struct Hit { int index = -1; bool onRightEdge = false; };
    Hit hitTest(const QPoint &pos) const;

    int rulerHeight() const { return 24; }
    int trackHeight() const { return 46; }
    int labelWidth()  const { return 120; }
    int tickWidth()   const { return m_tickWidth; }
    int ticksPerBar() const { return 16; }
    int snapTicks()   const { return 4; }

    int xToTick(int x) const;
    int tickToX(int t) const;
    int yToTrack(int y) const;
    int trackToY(int tr) const;

    void updateMinimumSize();

    Project    *m_project        = nullptr;
    QUndoStack *m_undoStack      = nullptr;
    int         m_activePattern  = 0;
    double      m_playheadTick   = 0.0;
    int         m_tickWidth      = 4;

    bool   m_panning  = false;
    QPoint m_panStart;

    int    m_dragIndex   = -1;
    bool   m_dragResize  = false;
    bool   m_dragging    = false;
    QPoint m_dragStart;
    int    m_origStart   = 0;
    int    m_origLength  = 0;
    int    m_origTrack   = 0;
    Clip   m_origClip;
    bool   m_dragChanged = false;
};
