#include "LanguageManager.h"
#include <QHash>
#include <QVector>

LanguageManager &LanguageManager::instance() {
    static LanguageManager inst;
    return inst;
}

// Карта переводов: ключ → [English, Russian, Chinese, PortugueseBR]
static const QHash<QString, QVector<QString>> &translationTable() {
    static const QHash<QString, QVector<QString>> t = {
        // Menu bar
        {"File",            {"File",             "Файл",             "文件",       "Arquivo"}},
        {"Edit",            {"Edit",             "Правка",           "编辑",       "Editar"}},
        {"Language",        {"Language",         "Язык",             "语言",       "Idioma"}},
        {"Help",            {"Help",             "Справка",          "帮助",       "Ajuda"}},

        // File menu
        {"Open Project...", {"Open Project...",  "Открыть проект...","打开项目...", "Abrir Projeto..."}},
        {"Save Project As...", {"Save Project As...","Сохранить проект...","项目另存为...","Salvar Projeto Como..."}},
        {"Import MIDI...",  {"Import MIDI...",   "Импорт MIDI...",   "导入MIDI...", "Importar MIDI..."}},
        {"Export SID...",   {"Export SID...",    "Экспорт SID...",   "导出SID...",  "Exportar SID..."}},
        {"Export WAV...",   {"Export WAV...",    "Экспорт WAV...",   "导出WAV...",  "Exportar WAV..."}},
        {"Quit",            {"Quit",             "Выход",            "退出",       "Sair"}},

        // Edit menu
        {"Undo",            {"Undo",             "Отменить",         "撤销",       "Desfazer"}},
        {"Redo",            {"Redo",             "Повторить",        "重做",       "Refazer"}},

        // Help menu
        {"About",           {"About",            "О программе",      "关于",       "Sobre"}},

        // Toolbar buttons
        {"Play",            {"Play",             "Играть",           "播放",       "Reproduzir"}},
        {"Pause",           {"Pause",            "Пауза",            "暂停",       "Pausar"}},
        {"Stop",            {"Stop",             "Стоп",             "停止",       "Parar"}},
        {"Record",          {"Record",           "Запись",           "录制",       "Gravar"}},
        {"Rename",          {"Rename",           "Переименовать",    "重命名",     "Renomear"}},
        {"Duplicate",       {"Duplicate",        "Дублировать",      "复制",       "Duplicar"}},
        {"Delete",          {"Delete",           "Удалить",          "删除",       "Excluir"}},
        {"New",             {"New",              "Создать",          "新建",       "Novo"}},
        {"Edit...",         {"Edit...",          "Изменить...",      "编辑...",    "Editar..."}},
        {"Refresh",         {"Refresh",          "Обновить",         "刷新",       "Atualizar"}},

        // Toolbar labels
        {"BPM:",            {"BPM:",             "BPM:",             "BPM:",       "BPM:"}},
        {"Pattern:",        {"Pattern:",         "Паттерн:",         "样式:",      "Padrão:"}},
        {"Master:",         {"Master:",          "Мастер:",          "主音量:",    "Principal:"}},
        {"MIDI In:",        {"MIDI In:",         "MIDI вход:",       "MIDI输入:",  "Entrada MIDI:"}},
        {"Rec Voice:",      {"Rec Voice:",       "Голос записи:",    "录制声部:",  "Voz de Gravação:"}},
        {"Instrument:",     {"Instrument:",      "Инструмент:",      "乐器:",      "Instrumento:"}},

        // Voices
        {"Voice 1",         {"Voice 1",          "Голос 1",          "声部 1",     "Voz 1"}},
        {"Voice 2",         {"Voice 2",          "Голос 2",          "声部 2",     "Voz 2"}},
        {"Voice 3",         {"Voice 3",          "Голос 3",          "声部 3",     "Voz 3"}},
        {"Voice 1 (lead)",  {"Voice 1 (lead)",   "Голос 1 (мелодия)","声部 1（主旋律）","Voz 1 (melodia)"}},
        {"Voice 2 (bass)",  {"Voice 2 (bass)",   "Голос 2 (бас)",    "声部 2（低音）","Voz 2 (baixo)"}},
        {"Voice 3 (harmony)", {"Voice 3 (harmony)","Голос 3 (гармония)","声部 3（和声）","Voz 3 (harmonia)"}},

        // MIDI port
        {"(no MIDI devices)", {"(no MIDI devices)","(нет MIDI устройств)","（无MIDI设备）","(sem dispositivos MIDI)"}},

        // Dock
        {"Instruments (drag to note)", {"Instruments (drag to note)","Инструменты (перетащить на ноту)","乐器（拖到音符）","Instrumentos (arraste até a nota)"}},
        {"Default (voice params)", {"Default (voice params)","По умолчанию (параметры голоса)","默认（声部参数）","Padrão (parâmetros da voz)"}},

        // About dialog
        {"About SID DAW 64", {"About SID DAW 64", "О программе SID DAW 64","关于 SID DAW 64","Sobre o SID DAW 64"}},
        {"Version", {"Version", "Версия", "版本", "Versão"}},
        {"Created by",      {"Created by",       "Автор",            "作者",       "Criado por"}},
        {"A DAW-style editor for the Commodore 64 SID chip.",
            {"A DAW-style editor for the Commodore 64 SID chip.",
             "DAW-редактор для звукового чипа Commodore 64 SID.",
             "Commodore 64 SID 芯片的 DAW 风格编辑器。",
             "Um editor no estilo DAW para o chip SID do Commodore 64."}},
        {"Create music with 3 SID voices, import MIDI, draw envelopes, and export to SID or WAV files.",
            {"Create music with 3 SID voices, import MIDI, draw envelopes, and export to SID or WAV files.",
             "Создавайте музыку на трёх голосах SID, импортируйте MIDI, рисуйте огибающие и экспортируйте в SID или WAV.",
             "用三个 SID 声部创作音乐，导入 MIDI，绘制包络，导出为 SID 或 WAV 文件。",
             "Crie música com 3 vozes SID, importe MIDI, desenhe envelopes e exporte para SID ou WAV."}},
        {"Built with Qt6, reSIDfp, RtMidi and 64tass.",
            {"Built with Qt6, reSIDfp, RtMidi and 64tass.",
             "Написано на Qt6, reSIDfp, RtMidi и 64tass.",
             "使用 Qt6、reSIDfp、RtMidi 和 64tass 构建。",
             "Construído com Qt6, reSIDfp, RtMidi e 64tass."}},

        // Language change message
        {"Language changed",
            {"Language changed", "Язык изменён", "语言已更改", "Idioma alterado"}},
        {"The interface language has been saved. Menu and About will update now. All other labels (dialogs, editors) will change after restarting the application.",
            {"The interface language has been saved. Menu and About will update now. All other labels (dialogs, editors) will change after restarting the application.",
             "Язык интерфейса сохранён. Меню и «О программе» обновятся сейчас. Остальные надписи (диалоги, редакторы) изменятся после перезапуска программы.",
             "界面语言已保存。菜单和关于将立即更新。其他标签（对话框、编辑器）将在重启应用后更改。",
             "O idioma da interface foi salvo. O menu e o Sobre serão atualizados agora. Outros rótulos (diálogos, editores) mudarão após reiniciar o aplicativo."}},

        // Language names (in their own language)
        {"English",  {"English",  "Английский", "英语",  "Inglês"}},
        {"Русский",  {"Русский",  "Русский",    "俄语",  "Russo"}},
        {"中文",     {"中文",     "Китайский",  "中文",  "Chinês"}},
        {"Português (BR)", {"Português (BR)", "Португальский (БР)", "葡萄牙语（巴西）", "Português (BR)"}},
    };
    return t;
}

QString LanguageManager::tr(const QString &key) {
    const auto &table = translationTable();
    auto it = table.find(key);
    if (it == table.end()) return key;
    int idx = int(instance().current());
    if (idx < 0 || idx >= it.value().size()) return key;
    return it.value()[idx];
}

QString LanguageManager::languageName(Language lang) {
    switch (lang) {
        case English:      return "English";
        case Russian:      return "Русский";
        case Chinese:      return "中文";
        case PortugueseBR: return "Português (BR)";
    }
    return "English";
}

QString LanguageManager::languageNativeName(Language lang) {
    return languageName(lang);
}

QVector<LanguageManager::Language> LanguageManager::availableLanguages() {
    return { English, Russian, Chinese, PortugueseBR };
}
