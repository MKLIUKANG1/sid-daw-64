#pragma once
#include <QWidget>
#include "Model.h"

class QComboBox;
class QSlider;
class QLabel;
class QCheckBox;
class QPushButton;
class QTabWidget;
class QSpinBox;
class QUndoStack;
class EnvelopeEditorWidget;

class VoicePanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit VoicePanelWidget(QWidget *parent = nullptr);

    void setPattern(Pattern *pat, int voiceIndex);
    void setVoiceParams(VoiceParams *vp);
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }

signals:
    void paramsChanged();

private slots:
    void onAnyChange();
    void onSavePreset();
    void onLoadPreset();
    void onEnvelopeChanged();
    void onClearEnvelope(int which);

private:
    void refreshFromModel();
    VoiceParams *currentParams() const;
    void wireEnvelope(EnvelopeEditorWidget *ed, int which);

    Pattern             *m_pattern    = nullptr;
    VoiceParams         *m_vp         = nullptr;
    QUndoStack          *m_undoStack  = nullptr;
    int                  m_voiceIndex = 0;
    bool                 m_updating   = false;

    VoiceParams          m_oldVPBeforeEdit;
    bool                 m_hasOldVP = false;

    QComboBox          *m_waveform   = nullptr;
    QSlider            *m_pulseW     = nullptr;
    QLabel             *m_pulseWLab  = nullptr;
    QSlider            *m_attack     = nullptr;
    QSlider            *m_decay      = nullptr;
    QSlider            *m_sustain    = nullptr;
    QSlider            *m_release    = nullptr;
    QCheckBox          *m_ring       = nullptr;
    QCheckBox          *m_sync       = nullptr;
    QPushButton        *m_saveBtn    = nullptr;
    QPushButton        *m_loadBtn    = nullptr;
    QSlider            *m_volume     = nullptr;
    QSlider            *m_pan        = nullptr;

    QTabWidget             *m_tabs      = nullptr;
    EnvelopeEditorWidget   *m_envVolume = nullptr;
    EnvelopeEditorWidget   *m_envPitch  = nullptr;
    EnvelopeEditorWidget   *m_envDuty   = nullptr;
    EnvelopeEditorWidget   *m_envArp    = nullptr;
    QSpinBox               *m_envSpeed  = nullptr;
};
