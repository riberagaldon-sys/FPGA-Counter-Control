#pragma once

#include <QWidget>

class QPainter;

class SevenSegmentDisplay final : public QWidget
{
    Q_OBJECT

public:
    explicit SevenSegmentDisplay(QWidget *parent = nullptr);

    quint32 value() const { return m_value; }
    void setValue(quint32 value);

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    static quint8 segmentMask(int hexadecimalDigit);
    void paintDigit(QPainter &painter, const QRectF &rect, int digit) const;

    quint32 m_value = 0;
};
