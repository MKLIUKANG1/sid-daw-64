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
class PreviewPlayer;
class WaveformPreview;

class VoicePanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit VoicePanelWidget(QWidget *parent = nullptr);

    void setPattern(Pattern *pat, int voiceIndex);
    void setVoiceParams(VoiceParams *vp);
    void setInstrument(Instrument *instr, QUndoStack *undo);
    void setUndoStack(QUndoStack *s) { m_undoStack = s; }

    void selectTab(int index);   // 0=Sliders, 1=Volume, 2=Pitch, 3=Duty, 4=Arp
    void triggerPreview();

signals:
    void paramsChanged();

private slots:
    void onAnyChange();
    void onSavePreset();
    void onLoadPreset();
    void onPreview();
    void onEnvelopeChanged();
    void onClearEnvelope(int which);
    void applyEnvelopePreset(int shape);

private:
    void refreshFromModel();
    VoiceParams *currentParams() const;
    void wireEnvelope(EnvelopeEditorWidget *ed, int which);
    static void setTooltip(QWidget *w, const QString &text);
    void pushUndo(const VoiceParams &oldVP, const VoiceParams &newVP);

    Pattern             *m_pattern    = nullptr;
    VoiceParams         *m_vp         = nullptr;
    Instrument          *m_instrument = nullptr;
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
    QPushButton        *m_previewBtn = nullptr;
    QSlider            *m_volume     = nullptr;
    QSlider            *m_pan        = nullptr;

    QTabWidget             *m_tabs      = nullptr;
    EnvelopeEditorWidget   *m_envVolume = nullptr;
    EnvelopeEditorWidget   *m_envPitch  = nullptr;
    EnvelopeEditorWidget   *m_envDuty   = nullptr;
    EnvelopeEditorWidget   *m_envArp    = nullptr;
    QSpinBox               *m_envSpeed  = nullptr;
    QComboBox              *m_pitchAbsoluteCombo = nullptr;
    QWidget                *m_pitchModeWidget = nullptr;
    QCheckBox              *m_autoPreviewCheck = nullptr;

    PreviewPlayer          *m_preview   = nullptr;
    WaveformPreview        *m_wavePreview = nullptr;
};
