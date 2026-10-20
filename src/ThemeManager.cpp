#include "ThemeManager.h"
#include <QApplication>
#include <QPalette>

ThemeManager& ThemeManager::instance() {
    static ThemeManager inst;
    return inst;
}

QString ThemeManager::themeName(Theme t) {
    switch (t) {
        case Dark:      return "Dark";
        case Light:     return "Light";
        case Midnight:  return "Midnight";
        case Solarized: return "Solarized";
    }
    return "Dark";
}

void ThemeManager::applyTheme(Theme t) {
    m_current = t;

    QPalette pal;
    switch (t) {
        case Dark:
            pal.setColor(QPalette::Window, QColor(45, 45, 48));
            pal.setColor(QPalette::WindowText, QColor(220, 220, 225));
            pal.setColor(QPalette::Base, QColor(30, 30, 34));
            pal.setColor(QPalette::AlternateBase, QColor(38, 38, 42));
            pal.setColor(QPalette::Text, QColor(220, 220, 225));
            pal.setColor(QPalette::Button, QColor(55, 55, 60));
            pal.setColor(QPalette::ButtonText, QColor(220, 220, 225));
            pal.setColor(QPalette::Highlight, QColor(80, 160, 255));
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::ToolTipBase, QColor(50, 50, 55));
            pal.setColor(QPalette::ToolTipText, QColor(230, 230, 235));
            break;
        case Light:
            pal.setColor(QPalette::Window, QColor(240, 240, 245));
            pal.setColor(QPalette::WindowText, QColor(20, 20, 25));
            pal.setColor(QPalette::Base, QColor(255, 255, 255));
            pal.setColor(QPalette::AlternateBase, QColor(245, 245, 250));
            pal.setColor(QPalette::Text, QColor(20, 20, 25));
            pal.setColor(QPalette::Button, QColor(225, 225, 230));
            pal.setColor(QPalette::ButtonText, QColor(20, 20, 25));
            pal.setColor(QPalette::Highlight, QColor(50, 120, 220));
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::ToolTipBase, QColor(255, 255, 230));
            pal.setColor(QPalette::ToolTipText, QColor(20, 20, 25));
            break;
        case Midnight:
            pal.setColor(QPalette::Window, QColor(15, 20, 35));
            pal.setColor(QPalette::WindowText, QColor(180, 200, 230));
            pal.setColor(QPalette::Base, QColor(10, 15, 28));
            pal.setColor(QPalette::AlternateBase, QColor(20, 28, 45));
            pal.setColor(QPalette::Text, QColor(180, 200, 230));
            pal.setColor(QPalette::Button, QColor(30, 40, 60));
            pal.setColor(QPalette::ButtonText, QColor(180, 200, 230));
            pal.setColor(QPalette::Highlight, QColor(100, 180, 255));
            pal.setColor(QPalette::HighlightedText, Qt::white);
            pal.setColor(QPalette::ToolTipBase, QColor(20, 30, 50));
            pal.setColor(QPalette::ToolTipText, QColor(200, 220, 240));
            break;
        case Solarized:
            pal.setColor(QPalette::Window, QColor(0, 43, 54));
            pal.setColor(QPalette::WindowText, QColor(147, 161, 161));
            pal.setColor(QPalette::Base, QColor(7, 54, 66));
            pal.setColor(QPalette::AlternateBase, QColor(0, 43, 54));
            pal.setColor(QPalette::Text, QColor(147, 161, 161));
            pal.setColor(QPalette::Button, QColor(7, 54, 66));
            pal.setColor(QPalette::ButtonText, QColor(131, 148, 150));
            pal.setColor(QPalette::Highlight, QColor(38, 139, 210));
            pal.setColor(QPalette::HighlightedText, QColor(253, 246, 227));
            break;
    }
    qApp->setPalette(pal);
    qApp->setStyleSheet(qssFor(t));
}

QString ThemeManager::qssFor(Theme t) const {
    QString bg, panel, border, text, textDim, hover, accent, btnBg, btnText;
    switch (t) {
        case Dark:
            bg="#2d2d30"; panel="#252528"; border="#3a3a3e";
            text="#dcdce1"; textDim="#999"; hover="#3d3d42";
            accent="#50a0ff"; btnBg="#37373c"; btnText="#dcdce1"; break;
        case Light:
            bg="#f0f0f5"; panel="#e8e8ee"; border="#c0c0c8";
            text="#141419"; textDim="#666"; hover="#dcdce4";
            accent="#3278dc"; btnBg="#e1e1e6"; btnText="#141419"; break;
        case Midnight:
            bg="#0f1423"; panel="#0a0f1c"; border="#1e2840";
            text="#b4c8e6"; textDim="#6b7d99"; hover="#1e2840";
            accent="#64b4ff"; btnBg="#1e2840"; btnText="#b4c8e6"; break;
        case Solarized:
            bg="#002b36"; panel="#073642"; border="#094555";
            text="#93a1a1"; textDim="#586e75"; hover="#094555";
            accent="#268bd2"; btnBg="#073642"; btnText="#93a1a1"; break;
    }

    return QString(R"(
QMainWindow, QDialog { background-color: %1; color: %4; }
QWidget { color: %4; }
QLabel { color: %4; background: transparent; }
QMenuBar { background-color: %2; color: %4; }
QMenuBar::item { padding: 4px 10px; background: transparent; }
QMenuBar::item:selected { background: %5; }
QMenu { background-color: %2; color: %4; border: 1px solid %3; }
QMenu::item { padding: 4px 20px; }
QMenu::item:selected { background: %6; color: white; }
QToolBar { background-color: %2; border: 0; spacing: 4px; padding: 2px; }
QToolButton { background-color: %7; color: %8; border: 1px solid %3; border-radius: 3px; padding: 4px 8px; }
QToolButton:hover { background-color: %5; }
QPushButton { background-color: %7; color: %8; border: 1px solid %3; border-radius: 3px; padding: 4px 10px; }
QPushButton:hover { background-color: %5; }
QPushButton:pressed, QPushButton:checked { background-color: %6; color: white; }
QLineEdit, QSpinBox, QDoubleSpinBox { background-color: %1; color: %4; border: 1px solid %3; border-radius: 3px; padding: 2px 4px; }
QComboBox { background-color: %7; color: %8; border: 1px solid %3; border-radius: 3px; padding: 2px 6px; }
QComboBox:hover { background-color: %5; }
QComboBox QAbstractItemView { background-color: %2; color: %4; selection-background-color: %6; selection-color: white; }
QScrollBar:vertical { background: %2; width: 12px; }
QScrollBar::handle:vertical { background: %3; border-radius: 6px; min-height: 20px; }
QScrollBar::handle:vertical:hover { background: %5; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: %2; height: 12px; }
QScrollBar::handle:horizontal { background: %3; border-radius: 6px; min-width: 20px; }
QScrollBar::handle:horizontal:hover { background: %5; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QTabWidget::pane { border: 1px solid %3; background: %2; }
QTabBar::tab { background: %7; color: %8; padding: 6px 12px; border: 1px solid %3; border-bottom: 0; }
QTabBar::tab:selected { background: %6; color: white; }
QTabBar::tab:hover { background: %5; }
QGroupBox { border: 1px solid %3; border-radius: 4px; margin-top: 8px; padding-top: 8px; color: %4; }
QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: %4; }
QDockWidget { color: %4; }
QDockWidget::title { background: %2; padding: 4px; }
QStatusBar { background: %2; color: %4; }
QListWidget { background: %1; color: %4; border: 1px solid %3; }
QListWidget::item { padding: 2px; }
QListWidget::item:selected { background: %6; color: white; }
QCheckBox { color: %4; }
QSlider::groove:horizontal { background: %3; height: 4px; border-radius: 2px; }
QSlider::handle:horizontal { background: %6; width: 12px; margin: -5px 0; border-radius: 6px; }
QSlider::handle:horizontal:hover { background: %5; }
QToolTip { background-color: %2; color: %4; border: 1px solid %3; padding: 3px; }
)").arg(bg, panel, border, text, hover, accent, btnBg, btnText);
}
