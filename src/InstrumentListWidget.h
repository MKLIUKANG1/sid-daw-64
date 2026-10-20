#pragma once
#include <QListWidget>
#include <QVector>
#include "Model.h"

class InstrumentListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit InstrumentListWidget(QWidget *parent = nullptr);

    void refreshFromProject(const Project &project);
    void setSelectedInstrument(int idx);

signals:
    void instrumentActivated(int instrIndex);
    void envelopeTabRequested(int instrIndex, int envType);
    void envelopeDropped(int instrIndex, int envType, QVector<int> env);

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void startDrag(Qt::DropActions) override;
};
