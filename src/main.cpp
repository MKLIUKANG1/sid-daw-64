#include <QApplication>
#include <QIcon>
#include <QSettings>
#include "LanguageManager.h"
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QApplication::setOrganizationName("MKLIUKANG1");
    QApplication::setApplicationName("SidDAW64");

    app.setWindowIcon(QIcon(":/icon.png"));

    // Загружаем сохранённый язык ДО создания UI
    QSettings settings;
    int lang = settings.value("language", int(LanguageManager::English)).toInt();
    if (lang < 0 || lang >= LanguageManager::availableLanguages().size())
        lang = int(LanguageManager::English);
    LanguageManager::instance().setCurrent(
        static_cast<LanguageManager::Language>(lang));

    MainWindow w;
    w.show();
    return app.exec();
}
