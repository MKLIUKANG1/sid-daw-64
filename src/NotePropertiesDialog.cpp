#include "NotePropertiesDialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDialogButtonBox>

NotePropertiesDialog::NotePropertiesDialog(const QVector<Note> &notes,
                                           const QVector<QString> &instrumentNames,
                                           QWidget *parent) : QDialog(parent) {
    setWindowTitle("Note Properties");
    resize(340, 300);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *info = new QLabel(QString("<b>%1</b> note(s) selected").arg(notes.size()));
    info->setTextFormat(Qt::RichText);
    root->addWidget(info);

    auto *form = new QFormLayout;
    form->setSpacing(6);

    m_instrCombo = new QComboBox;
    m_instrCombo->addItem("(default — use voice params)", -1);
    for (int i = 0; i < instrumentNames.size(); ++i)
        m_instrCombo->addItem(instrumentNames[i], i);
    form->addRow("Instrument:", m_instrCombo);

    m_voiceCombo = new QComboBox;
    m_voiceCombo->addItem("(don't change)", -1);
    m_voiceCombo->addItem("Voice 1 (lead)", 0);
    m_voiceCombo->addItem("Voice 2 (bass)", 1);
    m_voiceCombo->addItem("Voice 3 (harmony)", 2);
    form->addRow("Voice:", m_voiceCombo);

    m_pitchShift = new QSpinBox;
    m_pitchShift->setRange(-24, 24);
    m_pitchShift->setValue(0);
    m_pitchShift->setSuffix(" semitones");
    form->addRow("Pitch shift:", m_pitchShift);

    m_lengthDelta = new QSpinBox;
    m_lengthDelta->setRange(-32, 32);
    m_lengthDelta->setValue(0);
    m_lengthDelta->setSuffix(" ticks");
    form->addRow("Length delta:", m_lengthDelta);

    root->addLayout(form);
    root->addStretch();

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(bb);

    if (!notes.isEmpty()) {
        int idx = m_instrCombo->findData(notes[0].instrumentIndex);
        if (idx >= 0) m_instrCombo->setCurrentIndex(idx);
    }
}

int NotePropertiesDialog::instrumentIndex() const { return m_instrCombo->currentData().toInt(); }
int NotePropertiesDialog::voiceIndex() const { return m_voiceCombo->currentData().toInt(); }
int NotePropertiesDialog::pitchShift() const { return m_pitchShift->value(); }
int NotePropertiesDialog::lengthDelta() const { return m_lengthDelta->value(); }
