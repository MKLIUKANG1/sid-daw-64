#pragma once
#include <QWidget>
#include <QVector>

class EnvelopeEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit EnvelopeEditorWidget(QWidget *parent = nullptr);

    void setEnvelope(QVector<int> *env, int maxValue = 15);
    int  maxValue() const { return m_maxValue; }

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
    int m_maxValue = 15;
    int m_length   = 32;
    bool m_drawing = false;
};
