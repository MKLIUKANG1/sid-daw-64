#include <QApplication>
#include <QIcon>
#include <QSettings>
#include "LanguageManager.h"
#include "ThemeManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName("MKLIUKANG1");
    QApplication::setApplicationName("SidDAW64");
    app.setWindowIcon(QIcon(":/icon.png"));

    QSettings settings;

    int lang = settings.value("language", int(LanguageManager::English)).toInt();
    if (lang < 0 || lang >= LanguageManager::availableLanguages().size())
        lang = int(LanguageManager::English);
    LanguageManager::instance().setCurrent(
        static_cast<LanguageManager::Language>(lang));

    int themeInt = settings.value("theme", int(ThemeManager::Dark)).toInt();
    ThemeManager::instance().applyTheme(ThemeManager::fromInt(themeInt));

    MainWindow w;
    w.show();
    return app.exec();
}
