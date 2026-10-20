#pragma once
#include <QWidget>
#include <QVector>
#include <QPoint>

class EnvelopeEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EnvelopeEditorWidget(QWidget *parent = nullptr);

    void setEnvelope(QVector<int> *env, int maxValue, int defaultValue,
                     bool showCenter = false);
    void setEnvelopeType(int t) { m_envelopeType = t; }
    void setPoints(const QVector<int> &points);

signals:
    void envelopeChanged();
    void envelopeChangeStarted();
    void envelopeChangeFinished();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void updateLength();
    void ensureEnvelopeSize(int index);
    int valueToY(int value) const;
    int yToValue(int y) const;
    int xToIndex(int x) const;
    int indexToX(int index) const;

    QVector<int> *m_envelope = nullptr;
    int  m_maxValue     = 15;
    int  m_defaultValue = 15;
    bool m_showCenter   = false;
    int  m_length       = 32;
    bool m_drawing      = false;
    int  m_envelopeType = 0;      // 0=Vol, 1=Pitch, 2=Duty, 3=Arp
    QPoint m_dragStart;
    bool  m_dragPotential = false;
};
