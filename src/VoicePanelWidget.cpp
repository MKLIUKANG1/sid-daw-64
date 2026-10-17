#include "VoicePanelWidget.h"
#include "EnvelopeEditorWidget.h"
#include "VoicePresetIO.h"
#include "Commands.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QTabWidget>
#include <QComboBox>
#include <QSlider>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QUndoStack>

static QSlider *makeSlider(int min, int max, int def) {
    auto *s = new QSlider(Qt::Horizontal);
    s->setRange(min, max);
    s->setValue(def);
    s->setMinimumWidth(80);
    return s;
}

VoicePanelWidget::VoicePanelWidget(QWidget *parent) : QWidget(parent) {
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(4);

    m_tabs = new QTabWidget;

    // --- Sliders tab ---
    auto *slidersTab = new QWidget;
    auto *slidersLayout = new QVBoxLayout(slidersTab);
    slidersLayout->setContentsMargins(4, 4, 4, 4);
    slidersLayout->setSpacing(4);

    auto *waveRow = new QHBoxLayout;
    waveRow->addWidget(new QLabel("Wave:"));
    m_waveform = new QComboBox;
    m_waveform->addItem("Triangle", 0);
    m_waveform->addItem("Sawtooth", 1);
    m_waveform->addItem("Pulse",    2);
    m_waveform->addItem("Noise",    3);
    waveRow->addWidget(m_waveform, 1);
    slidersLayout->addLayout(waveRow);

    auto *pwRow = new QHBoxLayout;
    pwRow->addWidget(new QLabel("PW:"));
    m_pulseW = makeSlider(0, 4095, 2048);
    m_pulseWLab = new QLabel("2048");
    m_pulseWLab->setMinimumWidth(38);
    m_pulseWLab->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    pwRow->addWidget(m_pulseW, 1);
    pwRow->addWidget(m_pulseWLab);
    slidersLayout->addLayout(pwRow);

    auto *adsrBox = new QGroupBox("ADSR");
    auto *adsr = new QGridLayout(adsrBox);
    adsr->setSpacing(2);
    adsr->setContentsMargins(4, 4, 4, 4);
    adsr->addWidget(new QLabel("A"), 0, 0);
    m_attack = makeSlider(0, 15, 1);
    adsr->addWidget(m_attack, 0, 1);
    adsr->addWidget(new QLabel("D"), 1, 0);
    m_decay = makeSlider(0, 15, 0);
    adsr->addWidget(m_decay, 1, 1);
    adsr->addWidget(new QLabel("S"), 2, 0);
    m_sustain = makeSlider(0, 15, 15);
    adsr->addWidget(m_sustain, 2, 1);
    adsr->addWidget(new QLabel("R"), 3, 0);
    m_release = makeSlider(0, 15, 3);
    adsr->addWidget(m_release, 3, 1);
    slidersLayout->addWidget(adsrBox);

    auto *modRow = new QHBoxLayout;
    m_ring = new QCheckBox("Ring");
    m_sync = new QCheckBox("Sync");
    modRow->addWidget(m_ring);
    modRow->addWidget(m_sync);
    modRow->addStretch();
    slidersLayout->addLayout(modRow);

    auto *mixBox = new QGroupBox("Mixer");
    auto *mix = new QGridLayout(mixBox);
    mix->setSpacing(2);
    mix->setContentsMargins(4, 4, 4, 4);
    mix->addWidget(new QLabel("Vol"), 0, 0);
    m_volume = makeSlider(0, 100, 100);
    mix->addWidget(m_volume, 0, 1);
    mix->addWidget(new QLabel("Pan"), 1, 0);
    m_pan = makeSlider(0, 100, 50);
    mix->addWidget(m_pan, 1, 1);
    slidersLayout->addWidget(mixBox);

    auto *presetRow = new QHBoxLayout;
    m_saveBtn = new QPushButton("Save Preset");
    m_loadBtn = new QPushButton("Load Preset");
    presetRow->addWidget(m_saveBtn);
    presetRow->addWidget(m_loadBtn);
    slidersLayout->addLayout(presetRow);
    slidersLayout->addStretch();

    m_tabs->addTab(slidersTab, "Sliders");

    // --- Envelope tabs ---
    m_envVolume = new EnvelopeEditorWidget;
    m_envPitch  = new EnvelopeEditorWidget;
    m_envDuty   = new EnvelopeEditorWidget;
    m_envArp    = new EnvelopeEditorWidget;

    m_tabs->addTab(m_envVolume, "Volume");
    m_tabs->addTab(m_envPitch,  "Pitch");
    m_tabs->addTab(m_envDuty,   "Duty");
    m_tabs->addTab(m_envArp,    "Arp");

    root->addWidget(m_tabs, 1);

    // Speed / Clear row
    auto *ctlRow = new QHBoxLayout;
    ctlRow->addWidget(new QLabel("Speed:"));
    m_envSpeed = new QSpinBox;
    m_envSpeed->setRange(1, 16);
    m_envSpeed->setValue(4);
    m_envSpeed->setToolTip("Envelope points per tick");
    ctlRow->addWidget(m_envSpeed);
    ctlRow->addStretch();
    auto *clearBtn = new QPushButton("Clear Current");
    ctlRow->addWidget(clearBtn);
    root->addLayout(ctlRow);

    // connections: sliders
    connect(m_waveform, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VoicePanelWidget::onAnyChange);
    connect(m_pulseW, &QSlider::valueChanged, this, [this](int v) {
        m_pulseWLab->setText(QString::number(v));
        onAnyChange();
    });
    connect(m_attack,  &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_decay,   &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_sustain, &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_release, &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_ring,    &QCheckBox::toggled, this, &VoicePanelWidget::onAnyChange);
    connect(m_sync,    &QCheckBox::toggled, this, &VoicePanelWidget::onAnyChange);
    connect(m_volume,  &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_pan,     &QSlider::valueChanged, this, &VoicePanelWidget::onAnyChange);
    connect(m_envSpeed, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &VoicePanelWidget::onAnyChange);
    connect(m_saveBtn, &QPushButton::clicked, this, &VoicePanelWidget::onSavePreset);
    connect(m_loadBtn, &QPushButton::clicked, this, &VoicePanelWidget::onLoadPreset);

    // envelope connections
    wireEnvelope(m_envVolume, 0);
    wireEnvelope(m_envPitch,  1);
    wireEnvelope(m_envDuty,   2);
    wireEnvelope(m_envArp,    3);

    connect(clearBtn, &QPushButton::clicked, this, [this]() {
        int which = m_tabs->currentIndex() - 1;   // 0=Volume, 1=Pitch, 2=Duty, 3=Arp
        if (which >= 0 && which < 4) onClearEnvelope(which);
    });
}

void VoicePanelWidget::wireEnvelope(EnvelopeEditorWidget *ed, int which) {
    connect(ed, &EnvelopeEditorWidget::envelopeChanged,
            this, &VoicePanelWidget::onEnvelopeChanged);
    connect(ed, &EnvelopeEditorWidget::envelopeChangeStarted,
            this, [this]() {
        VoiceParams *vp = currentParams();
        if (vp) { m_oldVPBeforeEdit = *vp; m_hasOldVP = true; }
    });
    connect(ed, &EnvelopeEditorWidget::envelopeChangeFinished,
            this, [this]() {
        VoiceParams *vp = currentParams();
        if (!vp || !m_hasOldVP) return;
        m_hasOldVP = false;
        if (m_pattern && m_undoStack)
            m_undoStack->push(new ChangeVoiceParamCommand(
                m_pattern, m_voiceIndex, m_oldVPBeforeEdit, *vp));
        emit paramsChanged();
    });
    Q_UNUSED(which);
}

VoiceParams *VoicePanelWidget::currentParams() const {
    if (m_vp) return m_vp;
    if (m_pattern) return const_cast<VoiceParams*>(&m_pattern->voiceParams[m_voiceIndex]);
    return nullptr;
}

void VoicePanelWidget::setPattern(Pattern *pat, int voiceIndex) {
    m_pattern = pat;
    m_vp = nullptr;
    m_voiceIndex = voiceIndex;
    refreshFromModel();
}

void VoicePanelWidget::setVoiceParams(VoiceParams *vp) {
    m_vp = vp;
    m_pattern = nullptr;
    m_voiceIndex = 0;
    refreshFromModel();
}

void VoicePanelWidget::refreshFromModel() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    m_updating = true;
    m_waveform->setCurrentIndex(vp->waveform);
    m_pulseW->setValue(vp->pulseWidth);
    m_pulseWLab->setText(QString::number(vp->pulseWidth));
    m_attack->setValue(vp->attack);
    m_decay->setValue(vp->decay);
    m_sustain->setValue(vp->sustain);
    m_release->setValue(vp->release);
    m_ring->setChecked(vp->ringMod);
    m_sync->setChecked(vp->sync);
    m_volume->setValue(int(vp->volume * 100.0f));
    m_pan->setValue(int(vp->pan * 50.0f) + 50);
    m_envSpeed->setValue(vp->envelopeSpeed);

    m_envVolume->setEnvelope(&vp->volumeEnvelope,   15, 15, false);
    m_envPitch ->setEnvelope(&vp->pitchEnvelope,    15,  8, true);
    m_envDuty  ->setEnvelope(&vp->dutyEnvelope,     15,  8, true);
    m_envArp   ->setEnvelope(&vp->arpeggioEnvelope, 15,  0, false);

    m_updating = false;
}

void VoicePanelWidget::onAnyChange() {
    if (m_updating) return;
    VoiceParams *vp = currentParams();
    if (!vp) return;

    VoiceParams oldVP = *vp;
    vp->waveform   = m_waveform->currentIndex();
    vp->pulseWidth = m_pulseW->value();
    vp->attack     = m_attack->value();
    vp->decay      = m_decay->value();
    vp->sustain    = m_sustain->value();
    vp->release    = m_release->value();
    vp->ringMod    = m_ring->isChecked();
    vp->sync       = m_sync->isChecked();
    vp->volume     = m_volume->value() / 100.0f;
    vp->pan        = (m_pan->value() - 50) / 50.0f;
    vp->envelopeSpeed = m_envSpeed->value();

    if (m_pattern && m_undoStack)
        m_undoStack->push(new ChangeVoiceParamCommand(
            m_pattern, m_voiceIndex, oldVP, *vp));
    emit paramsChanged();
}

void VoicePanelWidget::onEnvelopeChanged() {
    emit paramsChanged();
}

void VoicePanelWidget::onClearEnvelope(int which) {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    VoiceParams oldVP = *vp;
    switch (which) {
        case 0: vp->volumeEnvelope.clear();   m_envVolume->setEnvelope(&vp->volumeEnvelope,   15, 15, false); break;
        case 1: vp->pitchEnvelope.clear();    m_envPitch ->setEnvelope(&vp->pitchEnvelope,    15,  8, true);  break;
        case 2: vp->dutyEnvelope.clear();     m_envDuty  ->setEnvelope(&vp->dutyEnvelope,     15,  8, true);  break;
        case 3: vp->arpeggioEnvelope.clear(); m_envArp   ->setEnvelope(&vp->arpeggioEnvelope, 15,  0, false); break;
    }
    if (m_pattern && m_undoStack)
        m_undoStack->push(new ChangeVoiceParamCommand(
            m_pattern, m_voiceIndex, oldVP, *vp));
    emit paramsChanged();
}

void VoicePanelWidget::onSavePreset() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    QString path = QFileDialog::getSaveFileName(this, "Save Voice Preset",
        QDir::homePath() + "/voice_preset.json", "Voice preset (*.json)");
    if (path.isEmpty()) return;
    if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".json";
    QString err = VoicePresetIO::save(*vp, path);
    if (!err.isEmpty()) QMessageBox::warning(this, "Save failed", err);
}

void VoicePanelWidget::onLoadPreset() {
    VoiceParams *vp = currentParams();
    if (!vp) return;
    QString path = QFileDialog::getOpenFileName(this, "Load Voice Preset",
        QDir::homePath(), "Voice preset (*.json)");
    if (path.isEmpty()) return;
    VoiceParams oldVP = *vp;
    VoiceParams loaded = oldVP;
    QString err = VoicePresetIO::load(loaded, path);
    if (!err.isEmpty()) { QMessageBox::warning(this, "Load failed", err); return; }
    *vp = loaded;
    refreshFromModel();
    if (m_pattern && m_undoStack)
        m_undoStack->push(new ChangeVoiceParamCommand(
            m_pattern, m_voiceIndex, oldVP, *vp));
    emit paramsChanged();
}
