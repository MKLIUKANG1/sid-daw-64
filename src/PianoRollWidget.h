#pragma once
#include <QWidget>
#include "Model.h"

struct Pattern;
class QUndoStack;

class PianoRollWidget : public QWidget {
    Q_OBJECT
public:
    explicit PianoRollWidget(QWidget *parent = nullptr);

    void setPattern(Pattern *pat);
    void setVoice(int v) { m_currentVoice = v; update(); }
    int  currentVoice() const { return m_currentVoice; }
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }

    void setZoom(int pixelsPerTick);
    int  zoom() const { return m_zoom; }

signals:
    void patternChanged();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;

private:
    struct Hit { int index = -1; bool onRightEdge = false; };
    Hit hitTest(const QPoint &pos) const;

    int rowHeight()    const { return 14; }
    int tickWidth()    const { return m_zoom; }
    int keyWidth()     const { return 48; }
    int lowestPitch()  const { return 36; }
    int highestPitch() const { return 84; }

    int yToPitch(int y) const;
    int pitchToY(int pitch) const;
    int xToTick(int x) const;
    int tickToX(int tick) const;

    void updateMinimumSize();

    Pattern     *m_pattern = nullptr;
    QUndoStack  *m_undoStack = nullptr;
    int m_currentVoice = 0;
    int m_zoom = 12;

    int    m_dragIndex   = -1;
    bool   m_dragResize  = false;
    bool   m_dragging    = false;
    QPoint m_dragStart;
    int    m_origTick    = 0;
    int    m_origLength  = 0;
    int    m_origPitch   = 0;
    Note   m_origNote;
    bool   m_dragChanged = false;
};
