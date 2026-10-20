#pragma once
#include <QWidget>
#include "Model.h"

class VoicePanelWidget;
class QPushButton;
class QUndoStack;

class InstrumentEditorPanel : public QWidget {
    Q_OBJECT
public:
    explicit InstrumentEditorPanel(QWidget *parent = nullptr);

    void setInstrument(Instrument *instr, QUndoStack *undo);
    void selectEnvelopeTab(int envType);

signals:
    void paramsChanged();

private:
    VoicePanelWidget *m_voicePanel = nullptr;
    QPushButton *m_btnSliders = nullptr;
    QPushButton *m_btnVolume  = nullptr;
    QPushButton *m_btnPitch   = nullptr;
    QPushButton *m_btnDuty    = nullptr;
    QPushButton *m_btnArp     = nullptr;
};
