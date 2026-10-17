#include "InstrumentListWidget.h"
#include <QDrag>
#include <QMimeData>

InstrumentListWidget::InstrumentListWidget(QWidget *parent) : QListWidget(parent) {
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDefaultDropAction(Qt::CopyAction);
}

void InstrumentListWidget::startDrag(Qt::DropActions) {
    auto *item = currentItem();
    if (!item) return;
    int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0) return;
    auto *mime = new QMimeData;
    mime->setData("application/x-siddaw-instrument", QByteArray::number(idx));
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->exec(Qt::CopyAction);
}
