#include "InstrumentEditorDialog.h"
#include "VoicePanelWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>

InstrumentEditorDialog::InstrumentEditorDialog(const Instrument &instr, QWidget *parent)
    : QDialog(parent), m_instr(instr) {
    setWindowTitle("Edit Instrument");
    resize(320, 640);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto *nameRow = new QHBoxLayout;
    nameRow->addWidget(new QLabel("Name:"));
    m_nameEdit = new QLineEdit(m_instr.name);
    nameRow->addWidget(m_nameEdit, 1);
    root->addLayout(nameRow);

    m_panel = new VoicePanelWidget;
    m_panel->setVoiceParams(&m_instr.params);
    root->addWidget(m_panel, 1);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &InstrumentEditorDialog::onAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void InstrumentEditorDialog::onAccept() {
    QString name = m_nameEdit->text().trimmed();
    if (!name.isEmpty())
        m_instr.name = name;
    accept();
}
