#include "InstrumentListWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QAbstractButton>
#include <QPainter>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDebug>

// ===== EnvelopeMiniButton: маленькая кнопка с превью огибающей =====
class EnvelopeMiniButton : public QAbstractButton {
public:
    explicit EnvelopeMiniButton(QWidget *parent = nullptr)
        : QAbstractButton(parent) {
        setFixedSize(30, 20);
        setCursor(Qt::PointingHandCursor);
    }

    void setLabel(const QString &l) { m_label = l; update(); }
    void setEnvelopeData(const QVector<int> &env, int maxValue,
                         int centerValue, bool showCenter) {
        m_env = env;
        m_max = std::max(1, maxValue);
        m_center = centerValue;
        m_showCenter = showCenter;
        update();
    }
    void setActive(bool a) { m_active = a; update(); }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor bg    = m_active ? QColor(42, 122, 42) : QColor(48, 48, 58);
        QColor bgHov = m_active ? QColor(58, 152, 58) : QColor(62, 62, 74);
        QColor border = m_active ? QColor(80, 170, 80) : QColor(72, 72, 82);

        if (underMouse()) bg = bgHov;
        p.fillRect(rect().adjusted(0, 0, -1, -1), bg);
        p.setPen(border);
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Буква слева
        QFont f = p.font();
        f.setPointSize(6);
        f.setBold(true);
        p.setFont(f);
        p.setPen(m_active ? Qt::white : QColor(120, 120, 130));
        p.drawText(QRect(2, 0, 9, height()),
                   Qt::AlignCenter, m_label);

        // Мини-кривая справа (10..28)
        int cx = 12;
        int cw = width() - cx - 3;
        int ch = height() - 4;
        QRect plot(cx, 2, cw, ch);
        p.fillRect(plot, QColor(20, 20, 24));

        if (m_showCenter) {
            int cy = plot.top() + plot.height() / 2;
            p.setPen(QPen(QColor(70, 70, 80), 1, Qt::DotLine));
            p.drawLine(plot.left(), cy, plot.right(), cy);
        }

        if (m_env.isEmpty()) {
            // пусто — пустой прямоугольник уже нарисован
            return;
        }

        QPolygon poly;
        int n = m_env.size();
        for (int i = 0; i < n; ++i) {
            double t = (n > 1) ? double(i) / (n - 1) : 0.0;
            int x = plot.left() + int(t * (plot.width() - 1));
            int v = std::clamp(m_env[i], 0, m_max);
            int y = plot.bottom() -
                    int(double(v) / m_max * (plot.height() - 1));
            poly << QPoint(x, y);
        }
        p.setPen(QPen(m_active ? QColor(200, 255, 200) : QColor(150, 200, 255),
                      1.5));
        p.setBrush(Qt::NoBrush);
        p.drawPolyline(poly);
    }

private:
    QVector<int> m_env;
    int m_max = 15;
    int m_center = 0;
    bool m_showCenter = false;
    QString m_label = "?";
    bool m_active = false;
};

// ===== InstrumentRowWidget =====
class InstrumentRowWidget : public QWidget {
    Q_OBJECT
public:
    InstrumentRowWidget(int row, QListWidget *owner,
                        Instrument *instr, const QColor &color,
                        QWidget *parent = nullptr)
        : QWidget(parent), m_row(row), m_owner(owner) {
        setAttribute(Qt::WA_TranslucentBackground);
        auto *l = new QHBoxLayout(this);
        l->setContentsMargins(4, 2, 4, 2);
        l->setSpacing(3);

        auto *colorBar = new QLabel;
        colorBar->setFixedSize(4, 18);
        colorBar->setStyleSheet(QString("background-color: %1;").arg(color.name()));
        l->addWidget(colorBar);

        auto *nameLbl = new QLabel(instr->name);
        nameLbl->setStyleSheet("color: #ddd;");
        l->addWidget(nameLbl, 1);

        auto addBtn = [this, l, instr](const QString &letter, int envType) {
            const QVector<int> *env = nullptr;
            bool showCenter = false;
            int defV = 15;
            switch (envType) {
                case 0: env = &instr->params.volumeEnvelope;   defV = 15; break;
                case 1: env = &instr->params.pitchEnvelope;    defV = 8;  showCenter = true; break;
                case 2: env = &instr->params.dutyEnvelope;     defV = 8;  showCenter = true; break;
                case 3: env = &instr->params.arpeggioEnvelope; defV = 0;  break;
            }
            auto *btn = new EnvelopeMiniButton(this);
            btn->setLabel(letter);
            btn->setEnvelopeData(*env, 15, defV, showCenter);
            btn->setActive(!env->isEmpty());

            static const char* names[4] = {"Volume", "Pitch", "Duty", "Arp"};
            btn->setToolTip(QString("Click to edit %1 envelope").arg(names[envType]));

            connect(btn, &QAbstractButton::clicked, this, [this, envType]() {
                if (m_owner) m_owner->setCurrentRow(m_row);
                emit envelopeClicked(envType);
            });
            l->addWidget(btn);
        };

        addBtn("V", 0);
        addBtn("P", 1);
        addBtn("D", 2);
        addBtn("A", 3);
    }

signals:
    void activated();
    void envelopeClicked(int envType);

protected:
    void mousePressEvent(QMouseEvent *e) override {
        if (m_owner) m_owner->setCurrentRow(m_row);
        QWidget::mousePressEvent(e);
    }
    void mouseDoubleClickEvent(QMouseEvent *e) override {
        if (m_owner) m_owner->setCurrentRow(m_row);
        emit activated();
        QWidget::mouseDoubleClickEvent(e);
    }

private:
    int m_row = 0;
    QListWidget *m_owner = nullptr;
};

// ===== InstrumentListWidget =====
InstrumentListWidget::InstrumentListWidget(QWidget *parent) : QListWidget(parent) {
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setDefaultDropAction(Qt::CopyAction);
    setSpacing(1);
}

void InstrumentListWidget::refreshFromProject(const Project &project) {
    clear();

    auto *defItem = new QListWidgetItem("Default (voice params)");
    defItem->setData(Qt::UserRole, -1);
    defItem->setForeground(QColor(160, 160, 160));
    addItem(defItem);

    static const QColor palette[8] = {
        QColor( 80, 160, 255), QColor(120, 220, 120),
        QColor(240, 160,  80), QColor(210, 120, 200),
        QColor( 90, 200, 200), QColor(255, 220, 100),
        QColor(200, 140, 255), QColor(150, 200, 130),
    };

    for (size_t i = 0; i < project.instruments.size(); ++i) {
        Instrument *instr = project.instruments[i].get();
        int row = count();
        auto *item = new QListWidgetItem();
        item->setData(Qt::UserRole, int(i));
        item->setSizeHint(QSize(0, 28));
        addItem(item);

        auto *w = new InstrumentRowWidget(row, this, instr,
                                          palette[i % 8]);
        setItemWidget(item, w);

        connect(w, &InstrumentRowWidget::activated, this, [this, i]() {
            emit instrumentActivated(int(i));
        });
        connect(w, &InstrumentRowWidget::envelopeClicked, this,
                [this, i](int envType) {
            emit envelopeTabRequested(int(i), envType);
        });
    }
}

void InstrumentListWidget::setSelectedInstrument(int idx) {
    for (int r = 0; r < count(); ++r) {
        if (item(r)->data(Qt::UserRole).toInt() == idx) {
            setCurrentRow(r);
            return;
        }
    }
}

void InstrumentListWidget::startDrag(Qt::DropActions) {
    // Drag from list items disabled — envelopes dragged from editor
}

void InstrumentListWidget::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasFormat("application/x-sid-envelope"))
        e->acceptProposedAction();
    else
        QListWidget::dragEnterEvent(e);
}

void InstrumentListWidget::dragMoveEvent(QDragMoveEvent *e) {
    if (e->mimeData()->hasFormat("application/x-sid-envelope")) {
        auto *item = itemAt(e->position().toPoint());
        if (item && item->data(Qt::UserRole).toInt() >= 0) {
            e->setDropAction(Qt::CopyAction);
            e->accept();
            return;
        }
        e->ignore();
        return;
    }
    QListWidget::dragMoveEvent(e);
}

void InstrumentListWidget::dropEvent(QDropEvent *e) {
    if (!e->mimeData()->hasFormat("application/x-sid-envelope")) {
        QListWidget::dropEvent(e);
        return;
    }
    auto *item = itemAt(e->position().toPoint());
    if (!item) { e->ignore(); return; }
    int instrIdx = item->data(Qt::UserRole).toInt();
    if (instrIdx < 0) { e->ignore(); return; }

    QByteArray data = e->mimeData()->data("application/x-sid-envelope");
    QString s = QString::fromUtf8(data);
    int sep = s.indexOf(':');
    if (sep < 0) { e->ignore(); return; }
    bool ok = false;
    int envType = s.left(sep).toInt(&ok);
    if (!ok) { e->ignore(); return; }

    QVector<int> env;
    QStringList parts = s.mid(sep + 1).split(',', Qt::SkipEmptyParts);
    for (const auto &p : parts) {
        int v = p.toInt(&ok);
        if (ok) env.append(v);
    }
    emit envelopeDropped(instrIdx, envType, env);
    e->acceptProposedAction();
}

#include "InstrumentListWidget.moc"
