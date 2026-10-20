#pragma once
#include <QString>

class ThemeManager {
public:
    enum Theme { Dark = 0, Light = 1, Midnight = 2, Solarized = 3 };

    static ThemeManager& instance();

    void applyTheme(Theme t);
    Theme current() const { return m_current; }
    void setCurrent(Theme t) { m_current = t; }

    static QString themeName(Theme t);
    static Theme fromInt(int v) {
        if (v < 0 || v > 3) return Dark;
        return static_cast<Theme>(v);
    }

private:
    ThemeManager() {}
    Theme m_current = Dark;
    QString qssFor(Theme t) const;
};
