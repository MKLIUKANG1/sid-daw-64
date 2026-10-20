#include "DrumEditorDialog.h"
#include "DrumPresets.h"
#include "InstrumentEditorDialog.h"
#include "Commands.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QUndoStack>

DrumEditorDialog::DrumEditorDialog(Project *project, QUndoStack *undo, QWidget *parent)
    : QDialog(parent), m_project(project), m_undo(undo) {
    setWindowTitle("Drum Kit Editor");
    resize(720, 520);

    auto *root = new QVBoxLayout(this);

    auto *info = new QLabel(
        "Map drum types (from MIDI channel 10) to instruments. "
        "Note pitch controls frequency; instrument controls waveform, ADSR and envelopes.");
    info->setWordWrap(true);
    root->addWidget(info);

    m_table = new QTableWidget;
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({"Drum", "MIDI Pitch", "Instrument", "Edit"});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table, 1);

    auto *btnRow = new QHBoxLayout;
    auto *addKit = new QPushButton("Add Default Kit");
    connect(addKit, &QPushButton::clicked, this, &DrumEditorDialog::onAddDefaultKit);
    btnRow->addWidget(addKit);
    btnRow->addStretch();
    root->addLayout(btnRow);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(bb);

    refreshTable();
}

void DrumEditorDialog::refreshTable() {
    m_table->setRowCount(m_project->drumKit.entries.size());
    for (int i = 0; i < m_project->drumKit.entries.size(); ++i) {
        const DrumSlot &slot = m_project->drumKit.entries[i];
        m_table->setItem(i, 0, new QTableWidgetItem(slot.name));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(slot.midiPitch)));

        auto *combo = new QComboBox;
        combo->addItem("(none — default noise)", -1);
        for (int j = 0; j < (int)m_project->instruments.size(); ++j)
            combo->addItem(m_project->instruments[j]->name, j);
        int cur = combo->findData(slot.instrumentIndex);
        if (cur >= 0) combo->setCurrentIndex(cur);
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this, i](int) { onChangeInstrument(i); });
        m_table->setCellWidget(i, 2, combo);

        auto *editBtn = new QPushButton("Edit...");
        connect(editBtn, &QPushButton::clicked, this, [this, i]() { onEditInstrument(i); });
        m_table->setCellWidget(i, 3, editBtn);
    }
}

void DrumEditorDialog::onAddDefaultKit() {
    auto defSlots = DrumPresets::defaultKitSlots();
    for (auto &s : defSlots) {
        bool present = false;
        for (const auto &existing : m_project->drumKit.entries)
            if (existing.midiPitch == s.midiPitch) { present = true; break; }
        if (present) continue;

        Instrument instr = DrumPresets::instrumentForPitch(s.midiPitch);
        m_undo->push(new AddInstrumentCommand(m_project, instr));
        s.instrumentIndex = (int)m_project->instruments.size() - 1;
        m_project->drumKit.entries.append(s);
    }
    refreshTable();
}

void DrumEditorDialog::onEditInstrument(int row) {
    if (row < 0 || row >= m_project->drumKit.entries.size()) return;
    DrumSlot &slot = m_project->drumKit.entries[row];

    if (slot.instrumentIndex < 0 ||
        slot.instrumentIndex >= (int)m_project->instruments.size()) {
        Instrument instr = DrumPresets::instrumentForPitch(slot.midiPitch);
        instr.name = slot.name;
        m_undo->push(new AddInstrumentCommand(m_project, instr));
        slot.instrumentIndex = (int)m_project->instruments.size() - 1;
    }

    Instrument *instr = m_project->instruments[slot.instrumentIndex].get();
    InstrumentEditorDialog dlg(*instr, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Instrument result = dlg.result();
    VoiceParams oldVP = instr->params;
    QString oldName = instr->name;
    instr->name = result.name;
    instr->params = result.params;
    if (result.name != oldName)
        m_undo->push(new RenameInstrumentCommand(instr, oldName, result.name));
    m_undo->push(new ChangeInstrumentParamsCommand(instr, oldVP, result.params));

    refreshTable();
}

void DrumEditorDialog::onChangeInstrument(int row) {
    if (row < 0 || row >= m_project->drumKit.entries.size()) return;
    auto *combo = qobject_cast<QComboBox*>(m_table->cellWidget(row, 2));
    if (!combo) return;
    m_project->drumKit.entries[row].instrumentIndex = combo->currentData().toInt();
}
