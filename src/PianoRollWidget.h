#pragma once
#include <QWidget>
#include <QSet>
#include <QVector>
#include "Model.h"

class QUndoStack;
struct Project;

class PianoRollWidget : public QWidget {
    Q_OBJECT
public:
    explicit PianoRollWidget(QWidget *parent = nullptr);

    void setPattern(Pattern *pat);
    void setProject(Project *p) { m_project = p; update(); }
    void setVoice(int v) { m_currentVoice = v; update(); }
    void setCurrentInstrument(int idx) { m_currentInstrument = idx; update(); }
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }
    void setZoom(int z);
    int  zoom() const { return m_zoom; }

signals:
    void patternChanged();
    void panBy(int dx, int dy);
    void noteInstrumentChanged(int instrumentIndex);
    void requestNoteDialog(const QVector<int> &indices);
    void instrumentDropped(const QVector<int> &indices, int instrumentIndex);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void keyPressEvent(QKeyEvent *) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

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
    void commitKeyboardMove(int pitchDelta, int tickDelta);
    void clearSelection();
    QColor colorForNote(const Note &n) const;

    Pattern     *m_pattern = nullptr;
    Project     *m_project = nullptr;
    QUndoStack  *m_undoStack = nullptr;
    int m_currentVoice = 0;
    int m_currentInstrument = -1;
    int m_zoom = 12;

    QSet<int> m_selected;
    bool   m_rubberBanding = false;
    QPoint m_rubberStart;
    QPoint m_rubberEnd;

    bool   m_panning  = false;
    QPoint m_panStart;

    int    m_dragIndex   = -1;
    bool   m_dragResize  = false;
    bool   m_dragging    = false;
    QPoint m_dragStart;
    int    m_origTick    = 0;
    int    m_origLength  = 0;
    int    m_origPitch   = 0;
    Note   m_origNote;
    bool   m_dragChanged = false;
    QVector<int>  m_dragIndices;
    QVector<Note> m_dragOldNotes;
};
