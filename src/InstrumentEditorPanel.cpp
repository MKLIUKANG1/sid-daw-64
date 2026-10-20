#include "InstrumentEditorPanel.h"
#include "VoicePanelWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QUndoStack>
#include <QShortcut>

InstrumentEditorPanel::InstrumentEditorPanel(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    auto *bar = new QHBoxLayout;
    auto *title = new QLabel("<b>Instrument Editor</b>");
    bar->addWidget(title);
    bar->addSpacing(12);

    m_btnSliders = new QPushButton("Sliders");
    m_btnVolume  = new QPushButton("Volume");
    m_btnPitch   = new QPushButton("Pitch");
    m_btnDuty    = new QPushButton("Duty");
    m_btnArp     = new QPushButton("Arp");
    for (auto *b : {m_btnSliders, m_btnVolume, m_btnPitch, m_btnDuty, m_btnArp}) {
        b->setCheckable(true);
        bar->addWidget(b);
    }
    m_btnSliders->setChecked(true);
    bar->addStretch();
    root->addLayout(bar);

    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    root->addWidget(line);

    m_voicePanel = new VoicePanelWidget;
    root->addWidget(m_voicePanel, 1);

    auto select = [this](int idx, QPushButton *b) {
        m_voicePanel->selectTab(idx);
        for (auto *other : {m_btnSliders, m_btnVolume, m_btnPitch, m_btnDuty, m_btnArp})
            other->setChecked(other == b);
    };
    connect(m_btnSliders, &QPushButton::clicked, this, [this, select]() { select(0, m_btnSliders); });
    connect(m_btnVolume,  &QPushButton::clicked, this, [this, select]() { select(1, m_btnVolume); });
    connect(m_btnPitch,   &QPushButton::clicked, this, [this, select]() { select(2, m_btnPitch); });
    connect(m_btnDuty,    &QPushButton::clicked, this, [this, select]() { select(3, m_btnDuty); });
    connect(m_btnArp,     &QPushButton::clicked, this, [this, select]() { select(4, m_btnArp); });
    // Hotkeys 1-4 → switch envelope tabs
    auto addShortcut = [this](int key, QPushButton *btn) {
        auto *sc = new QShortcut(QKeySequence(QString::number(key)), this);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, [this, btn]() {
            if (btn) btn->click();
        });
    };
    addShortcut(1, m_btnVolume);
    addShortcut(2, m_btnPitch);
    addShortcut(3, m_btnDuty);
    addShortcut(4, m_btnArp);
    connect(m_voicePanel, &VoicePanelWidget::paramsChanged,
            this, &InstrumentEditorPanel::paramsChanged);
}

void InstrumentEditorPanel::setInstrument(Instrument *instr, QUndoStack *undo) {
    m_voicePanel->setInstrument(instr, undo);
}

void InstrumentEditorPanel::selectEnvelopeTab(int envType) {
    QPushButton *btn = nullptr;
    switch (envType) {
        case 0: btn = m_btnVolume; break;
        case 1: btn = m_btnPitch;  break;
        case 2: btn = m_btnDuty;   break;
        case 3: btn = m_btnArp;    break;
    }
    if (btn) btn->click();
}