#pragma once
#include <QListWidget>

class InstrumentListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit InstrumentListWidget(QWidget *parent = nullptr);
protected:
    void startDrag(Qt::DropActions supportedActions) override;
};
