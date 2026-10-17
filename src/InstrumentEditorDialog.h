#pragma once
#include <QDialog>
#include "Model.h"

class QLineEdit;
class VoicePanelWidget;

class InstrumentEditorDialog : public QDialog {
    Q_OBJECT
public:
    explicit InstrumentEditorDialog(const Instrument &instr, QWidget *parent = nullptr);

    Instrument result() const { return m_instr; }

private slots:
    void onAccept();

private:
    Instrument         m_instr;
    QLineEdit         *m_nameEdit = nullptr;
    VoicePanelWidget  *m_panel    = nullptr;
};
