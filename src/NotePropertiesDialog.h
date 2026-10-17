#pragma once
#include <QDialog>
#include <QVector>
#include "Model.h"

class QComboBox;
class QSpinBox;

class NotePropertiesDialog : public QDialog {
    Q_OBJECT
public:
    NotePropertiesDialog(const QVector<Note> &notes,
                         const QVector<QString> &instrumentNames,
                         QWidget *parent = nullptr);

    int  instrumentIndex() const;
    int  voiceIndex() const;
    int  pitchShift() const;
    int  lengthDelta() const;

private:
    QComboBox *m_instrCombo  = nullptr;
    QComboBox *m_voiceCombo  = nullptr;
    QSpinBox  *m_pitchShift  = nullptr;
    QSpinBox  *m_lengthDelta = nullptr;
};
