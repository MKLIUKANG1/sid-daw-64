#pragma once
#include <QDialog>
#include <QVector>
#include <QHash>
#include "MidiImporter.h"

class QRadioButton;
class QPushButton;

class MidiImportDialog : public QDialog {
    Q_OBJECT
public:
    enum Mode { SinglePattern, SeparatePatterns };

    MidiImportDialog(const QString &fileName,
                     int bpm, int lengthTicks, int noteCount,
                     const QVector<ChannelInfo> &channels,
                     QWidget *parent = nullptr);

    Mode mode() const;
    QHash<int,int> channelMapping() const; // midi ch -> voice (0..2), -1 = skip

private slots:
    void onChannelToggled();
    void onModeChanged();

private:
    Mode m_mode = SinglePattern;

    struct Row {
        int midiChannel = 0;
        class QCheckBox  *checkbox = nullptr;
        class QComboBox  *voiceCombo = nullptr;
    };
    QVector<Row> m_rows;

    QRadioButton *m_single   = nullptr;
    QRadioButton *m_separate = nullptr;
    QPushButton  *m_okBtn    = nullptr;

    void updateOkState();
};
