#pragma once
#include <QString>
#include <QVector>

class LanguageManager {
public:
    enum Language {
        English = 0,
        Russian = 1,
        Chinese = 2,
        PortugueseBR = 3
    };

    static LanguageManager &instance();

    Language current() const { return m_current; }
    void setCurrent(Language lang) { m_current = lang; }

    // Перевод по ключу. Если ключа нет — возвращает сам ключ.
    static QString tr(const QString &key);

    static QString languageName(Language lang);
    static QString languageNativeName(Language lang);
    static QVector<Language> availableLanguages();

private:
    LanguageManager() {}
    Language m_current = English;
};
