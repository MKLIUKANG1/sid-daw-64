#include "AboutDialog.h"
#include "LanguageManager.h"
#include "version.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QFrame>

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(LanguageManager::tr("About SID DAW 64"));
    resize(520, 360);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto *title = new QLabel("<h1 style='margin:0;'>SID DAW 64</h1>");
    title->setTextFormat(Qt::RichText);
    root->addWidget(title);

    auto *version = new QLabel(
        "<span style='color:#888;'>"
        + LanguageManager::tr("Version") + " " + SIDDAW_VERSION
        + "</span>");
    version->setTextFormat(Qt::RichText);
    root->addWidget(version);

    auto *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    root->addWidget(line);

    auto *creator = new QLabel(
        "<b>" + LanguageManager::tr("Created by") + ":</b> MKLIUKANG1");
    creator->setTextFormat(Qt::RichText);
    root->addWidget(creator);

    root->addSpacing(6);

    auto *desc = new QLabel(LanguageManager::tr(
        "A DAW-style editor for the Commodore 64 SID chip."));
    desc->setWordWrap(true);
    root->addWidget(desc);

    auto *desc2 = new QLabel(LanguageManager::tr(
        "Create music with 3 SID voices, import MIDI, draw envelopes, "
        "and export to SID or WAV files."));
    desc2->setWordWrap(true);
    root->addWidget(desc2);

    root->addSpacing(6);

    auto *tech = new QLabel(LanguageManager::tr(
        "Built with Qt6, reSIDfp, RtMidi and 64tass."));
    tech->setStyleSheet("color: #999;");
    tech->setWordWrap(true);
    root->addWidget(tech);

    root->addStretch();

    auto *link = new QLabel(
        "<a href=\"https://github.com/MKLIUKANG1/sid-daw-64\" "
        "style=\"color:#5aa;\">github.com/MKLIUKANG1/sid-daw-64</a>");
    link->setOpenExternalLinks(true);
    link->setTextFormat(Qt::RichText);
    root->addWidget(link);

    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    root->addWidget(bb);
}
