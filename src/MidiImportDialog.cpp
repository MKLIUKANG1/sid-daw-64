#include "MidiImportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QFileInfo>

MidiImportDialog::MidiImportDialog(const QString &fileName,
                                   int bpm, int lengthTicks, int noteCount,
                                   const QVector<ChannelInfo> &channels,
                                   QWidget *parent) : QDialog(parent) {
    setWindowTitle("Import MIDI");
    resize(620, 440);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    QFileInfo fi(fileName);
    auto *info = new QLabel(
        QString("<b>%1</b><br>BPM %2 &middot; %3 ticks &middot; %4 notes &middot; %5 channel(s)")
            .arg(fi.fileName()).arg(bpm).arg(lengthTicks)
            .arg(noteCount).arg(channels.size()));
    info->setTextFormat(Qt::RichText);
    root->addWidget(info);

    // Mode
    auto *modeBox = new QGroupBox("Import mode");
    auto *modeLayout = new QVBoxLayout(modeBox);
    m_single   = new QRadioButton("Single pattern (all selected channels combined)");
    m_separate = new QRadioButton("Separate pattern per channel (one track each)");
    m_single->setChecked(true);
    modeLayout->addWidget(m_single);
    modeLayout->addWidget(m_separate);
    root->addWidget(modeBox);

    connect(m_single,   &QRadioButton::toggled,
            this, &MidiImportDialog::onModeChanged);
    connect(m_separate, &QRadioButton::toggled,
            this, &MidiImportDialog::onModeChanged);

    // Channels
    auto *chBox = new QGroupBox("Channels");
    auto *grid = new QGridLayout(chBox);
    grid->setSpacing(4);
    grid->addWidget(new QLabel("<b>Import</b>"), 0, 0);
    grid->addWidget(new QLabel("<b>Channel</b>"), 0, 1);
    grid->addWidget(new QLabel("<b>Notes / Range</b>"), 0, 2);
    grid->addWidget(new QLabel("<b>SID Voice</b>"), 0, 3);

    // Detect drums present
    bool hasDrums = false;
    for (const auto &ci : channels)
        if (ci.channel == 9) hasDrums = true;

    int row = 1;
    int melodicIdx = 0;
    for (const auto &ci : channels) {
        Row r;
        r.midiChannel = ci.channel;

        r.checkbox = new QCheckBox;
        r.checkbox->setChecked(true);   // все каналы включены, включая drums
        grid->addWidget(r.checkbox, row, 0);

        QString chName = QString("Ch %1").arg(ci.channel + 1);
        if (ci.channel == 9) chName += " — drums (auto Noise)";
        grid->addWidget(new QLabel(chName), row, 1);

        QString range;
        if (ci.minPitch <= ci.maxPitch) {
            range = QString("%1 notes   %2..%3")
                        .arg(ci.noteCount)
                        .arg(ci.minPitch).arg(ci.maxPitch);
        } else {
            range = QString("%1 notes").arg(ci.noteCount);
        }
        grid->addWidget(new QLabel(range), row, 2);

        r.voiceCombo = new QComboBox;
        r.voiceCombo->addItem("Voice 1 (lead)",    0);
        r.voiceCombo->addItem("Voice 2 (bass)",    1);
        r.voiceCombo->addItem("Voice 3 (harmony)", 2);

        if (ci.channel == 9) {
            r.voiceCombo->setCurrentIndex(2);   // барабаны → Voice 3
        } else {
            // Мелодические каналы: сначала 0 и 1 (не трогаем Voice 3 при наличии барабанов)
            int v;
            if (hasDrums) {
                if (melodicIdx == 0)      v = 0;
                else if (melodicIdx == 1) v = 1;
                else                      v = 2;   // переполнение
            } else {
                v = melodicIdx % 3;
            }
            r.voiceCombo->setCurrentIndex(v);
            melodicIdx++;
        }
        grid->addWidget(r.voiceCombo, row, 3);

        connect(r.checkbox, &QCheckBox::toggled,
                this, &MidiImportDialog::onChannelToggled);

        m_rows.append(r);
        row++;
    }
    root->addWidget(chBox);
    root->addStretch();

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_okBtn = buttons->button(QDialogButtonBox::Ok);
    m_okBtn->setText("Import");
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);

    onModeChanged();
    updateOkState();
}

MidiImportDialog::Mode MidiImportDialog::mode() const {
    return m_separate->isChecked() ? SeparatePatterns : SinglePattern;
}

QHash<int,int> MidiImportDialog::channelMapping() const {
    QHash<int,int> map;
    for (const auto &r : m_rows) {
        if (!r.checkbox->isChecked()) {
            map[r.midiChannel] = -1;
            continue;
        }
        map[r.midiChannel] = r.voiceCombo->currentData().toInt();
    }
    return map;
}

void MidiImportDialog::onChannelToggled() {
    updateOkState();
}

void MidiImportDialog::onModeChanged() {
    bool separate = m_separate->isChecked();
    for (auto &r : m_rows)
        r.voiceCombo->setEnabled(r.checkbox->isChecked());
    Q_UNUSED(separate);
    updateOkState();
}

void MidiImportDialog::updateOkState() {
    bool any = false;
    for (const auto &r : m_rows)
        if (r.checkbox->isChecked()) { any = true; break; }
    m_okBtn->setEnabled(any);
}