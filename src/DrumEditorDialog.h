#pragma once
#include <QDialog>
#include "Model.h"

class QTableWidget;
class QUndoStack;

class DrumEditorDialog : public QDialog {
    Q_OBJECT
public:
    DrumEditorDialog(Project *project, QUndoStack *undo, QWidget *parent = nullptr);

private slots:
    void refreshTable();
    void onAddDefaultKit();
    void onEditInstrument(int row);
    void onChangeInstrument(int row);

private:
    Project *m_project;
    QUndoStack *m_undo;
    QTableWidget *m_table;
};
