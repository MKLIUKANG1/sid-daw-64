#pragma once
#include <QUndoCommand>
#include <QVector>
#include <algorithm>
#include "Model.h"

class AddNoteCommand : public QUndoCommand {
public:
    AddNoteCommand(Pattern *pat, const Note &n)
        : QUndoCommand("Add note"), m_pat(pat), m_note(n) {}
    void undo() override { m_pat->notes.removeLast(); }
    void redo() override { m_pat->notes.append(m_note); }
private:
    Pattern *m_pat;
    Note m_note;
};

class RemoveNoteCommand : public QUndoCommand {
public:
    RemoveNoteCommand(Pattern *pat, int index)
        : QUndoCommand("Delete note"), m_pat(pat), m_index(index) {
        m_note = m_pat->notes[index];
    }
    void undo() override { m_pat->notes.insert(m_index, m_note); }
    void redo() override { m_pat->notes.removeAt(m_index); }
private:
    Pattern *m_pat;
    int m_index;
    Note m_note;
};

class ModifyNoteCommand : public QUndoCommand {
public:
    ModifyNoteCommand(Pattern *pat, int index, const Note &oldN, const Note &newN)
        : QUndoCommand("Modify note"), m_pat(pat), m_index(index),
          m_old(oldN), m_new(newN) {}
    void undo() override { m_pat->notes[m_index] = m_old; }
    void redo() override { m_pat->notes[m_index] = m_new; }
private:
    Pattern *m_pat;
    int m_index;
    Note m_old, m_new;
};

class MoveNotesCommand : public QUndoCommand {
public:
    MoveNotesCommand(Pattern *pat, const QVector<int> &indices,
                     const QVector<Note> &oldNotes, const QVector<Note> &newNotes)
        : QUndoCommand("Move notes"), m_pat(pat), m_indices(indices),
          m_old(oldNotes), m_new(newNotes) {}
    void undo() override {
        for (int i = 0; i < m_indices.size(); ++i)
            m_pat->notes[m_indices[i]] = m_old[i];
    }
    void redo() override {
        for (int i = 0; i < m_indices.size(); ++i)
            m_pat->notes[m_indices[i]] = m_new[i];
    }
private:
    Pattern *m_pat;
    QVector<int> m_indices;
    QVector<Note> m_old, m_new;
};

class RemoveNotesCommand : public QUndoCommand {
public:
    RemoveNotesCommand(Pattern *pat, QVector<int> indices)
        : QUndoCommand("Delete notes"), m_pat(pat), m_indices(indices) {
        std::sort(m_indices.begin(), m_indices.end());
        for (int i : m_indices)
            m_notes.append(m_pat->notes[i]);
    }
    void undo() override {
        for (int i = 0; i < m_indices.size(); ++i)
            m_pat->notes.insert(m_indices[i], m_notes[i]);
    }
    void redo() override {
        for (int i = m_indices.size() - 1; i >= 0; --i)
            m_pat->notes.removeAt(m_indices[i]);
    }
private:
    Pattern *m_pat;
    QVector<int> m_indices;
    QVector<Note> m_notes;
};

class ChangeNoteInstrumentCommand : public QUndoCommand {
public:
    ChangeNoteInstrumentCommand(Pattern *pat, QVector<int> indices, int newInstr)
        : QUndoCommand("Change note instrument"),
          m_pat(pat), m_indices(indices), m_new(newInstr) {
        for (int i : m_indices)
            m_old.append(m_pat->notes[i].instrumentIndex);
    }
    void undo() override {
        for (int k = 0; k < m_indices.size(); ++k)
            m_pat->notes[m_indices[k]].instrumentIndex = m_old[k];
    }
    void redo() override {
        for (int i : m_indices)
            m_pat->notes[i].instrumentIndex = m_new;
    }
private:
    Pattern *m_pat;
    QVector<int> m_indices;
    QVector<int> m_old;
    int m_new;
};

class AddClipCommand : public QUndoCommand {
public:
    AddClipCommand(Project *proj, const Clip &c)
        : QUndoCommand("Add clip"), m_proj(proj), m_clip(c) {}
    void undo() override { m_proj->clips.removeLast(); }
    void redo() override { m_proj->clips.append(m_clip); }
private:
    Project *m_proj;
    Clip m_clip;
};

class RemoveClipCommand : public QUndoCommand {
public:
    RemoveClipCommand(Project *proj, int index)
        : QUndoCommand("Delete clip"), m_proj(proj), m_index(index) {
        m_clip = m_proj->clips[index];
    }
    void undo() override { m_proj->clips.insert(m_index, m_clip); }
    void redo() override { m_proj->clips.removeAt(m_index); }
private:
    Project *m_proj;
    int m_index;
    Clip m_clip;
};

class ModifyClipCommand : public QUndoCommand {
public:
    ModifyClipCommand(Project *proj, int index, const Clip &oldC, const Clip &newC)
        : QUndoCommand("Modify clip"), m_proj(proj), m_index(index),
          m_old(oldC), m_new(newC) {}
    void undo() override { m_proj->clips[m_index] = m_old; }
    void redo() override { m_proj->clips[m_index] = m_new; }
private:
    Project *m_proj;
    int m_index;
    Clip m_old, m_new;
};

class ChangeVoiceParamCommand : public QUndoCommand {
public:
    ChangeVoiceParamCommand(Pattern *pat, int voice,
                            const VoiceParams &oldVP, const VoiceParams &newVP)
        : QUndoCommand("Change voice parameters"),
          m_pat(pat), m_voice(voice), m_old(oldVP), m_new(newVP) {}
    void undo() override { m_pat->voiceParams[m_voice] = m_old; }
    void redo() override { m_pat->voiceParams[m_voice] = m_new; }
    int id() const override { return 1001; }
    bool mergeWith(const QUndoCommand *other) override {
        if (other->id() != 1001) return false;
        auto *o = static_cast<const ChangeVoiceParamCommand*>(other);
        if (o->m_pat != m_pat || o->m_voice != m_voice) return false;
        m_new = o->m_new;
        return true;
    }
private:
    Pattern *m_pat;
    int m_voice;
    VoiceParams m_old, m_new;
};

class AddInstrumentCommand : public QUndoCommand {
public:
    AddInstrumentCommand(Project *proj, const Instrument &instr)
        : QUndoCommand("Add instrument"), m_proj(proj), m_instrCopy(instr) {}
    void undo() override { m_proj->instruments.pop_back(); }
    void redo() override {
        m_proj->instruments.push_back(std::make_unique<Instrument>(m_instrCopy));
    }
private:
    Project *m_proj;
    Instrument m_instrCopy;
};

class RemoveInstrumentCommand : public QUndoCommand {
public:
    RemoveInstrumentCommand(Project *proj, int index)
        : QUndoCommand("Delete instrument"), m_proj(proj), m_index(index) {
        m_instrCopy = *m_proj->instruments[index];
    }
    void undo() override {
        m_proj->instruments.insert(
            m_proj->instruments.begin() + m_index,
            std::make_unique<Instrument>(m_instrCopy));
    }
    void redo() override {
        m_proj->instruments.erase(m_proj->instruments.begin() + m_index);
    }
private:
    Project *m_proj;
    int m_index;
    Instrument m_instrCopy;
};

class RenameInstrumentCommand : public QUndoCommand {
public:
    RenameInstrumentCommand(Instrument *instr,
                            const QString &oldName, const QString &newName)
        : QUndoCommand("Rename instrument"),
          m_instr(instr), m_old(oldName), m_new(newName) {}
    void undo() override { m_instr->name = m_old; }
    void redo() override { m_instr->name = m_new; }
private:
    Instrument *m_instr;
    QString m_old, m_new;
};

class ChangeInstrumentParamsCommand : public QUndoCommand {
public:
    ChangeInstrumentParamsCommand(Instrument *instr,
                                  const VoiceParams &oldVP,
                                  const VoiceParams &newVP)
        : QUndoCommand("Change instrument parameters"),
          m_instr(instr), m_old(oldVP), m_new(newVP) {}
    void undo() override { m_instr->params = m_old; }
    void redo() override { m_instr->params = m_new; }
private:
    Instrument *m_instr;
    VoiceParams m_old, m_new;
};

class RenamePatternCommand : public QUndoCommand {
public:
    RenamePatternCommand(Pattern *pat, const QString &oldName, const QString &newName)
        : QUndoCommand("Rename pattern"), m_pat(pat),
          m_old(oldName), m_new(newName) {}
    void undo() override { m_pat->name = m_old; }
    void redo() override { m_pat->name = m_new; }
private:
    Pattern *m_pat;
    QString m_old, m_new;
};

class AddPatternCommand : public QUndoCommand {
public:
    AddPatternCommand(Project *proj, const Pattern &pat)
        : QUndoCommand("Add pattern"), m_proj(proj), m_patCopy(pat) {}
    void undo() override { m_proj->patterns.pop_back(); }
    void redo() override {
        m_proj->patterns.push_back(std::make_unique<Pattern>(m_patCopy));
    }
private:
    Project *m_proj;
    Pattern m_patCopy;
};

class DeletePatternCommand : public QUndoCommand {
public:
    DeletePatternCommand(Project *proj, int index)
        : QUndoCommand("Delete pattern"), m_proj(proj), m_index(index) {
        m_patCopy     = *m_proj->patterns[index];
        m_clipsBefore = m_proj->clips;
    }
    void undo() override {
        m_proj->patterns.insert(m_proj->patterns.begin() + m_index,
                                std::make_unique<Pattern>(m_patCopy));
        m_proj->clips = m_clipsBefore;
    }
    void redo() override {
        QVector<Clip> newClips;
        for (const auto &c : m_clipsBefore) {
            if (c.patternIndex != m_index) {
                Clip c2 = c;
                if (c2.patternIndex > m_index) c2.patternIndex--;
                newClips.append(c2);
            }
        }
        m_proj->clips = newClips;
        m_proj->patterns.erase(m_proj->patterns.begin() + m_index);
    }
private:
    Project *m_proj;
    int m_index;
    Pattern m_patCopy;
    QVector<Clip> m_clipsBefore;
};
