#pragma once
#include <QWidget>
#include "Model.h"

class QUndoStack;

class DrumEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit DrumEditorWidget(QWidget *parent = nullptr);

    void setPattern(Pattern *pat);
    void setProject(Project *proj) { m_project = proj; updateMinimumSize(); update(); }
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }
    void setCurrentVoice(int v) { m_currentVoice = v; }

signals:
    void patternChanged();
    void panBy(int dx, int dy);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    int rowHeight()  const { return 24; }
    int tickWidth()  const { return 20; }
    int labelWidth() const { return 120; }
    int rowCount()   const;
    int rowPitch(int row) const;
    int rowInstrument(int row) const;

    int yToRow(int y) const;
    int xToTick(int x) const;
    int tickToX(int t) const;
    int rowToY(int r) const;

    int findNoteAt(int row, int tick) const;
    void updateMinimumSize();
    void ensureInstrumentForRow(int row);

    Pattern *m_pattern = nullptr;
    Project *m_project = nullptr;
    QUndoStack *m_undoStack = nullptr;
    int m_currentVoice = 2;   // Default to Voice 3 for drums

    bool m_panning = false;
    QPoint m_panStart;
};
