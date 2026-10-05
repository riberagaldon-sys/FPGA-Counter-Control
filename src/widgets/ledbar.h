#pragma once

#include <QWidget>

class LedBar final : public QWidget
{
    Q_OBJECT

public:
    explicit LedBar(QWidget *parent = nullptr);
    void setMask(quint16 mask);
    quint16 mask() const { return m_mask; }

protected:
    void paintEvent(QPaintEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

private:
    quint16 m_mask = 0x001U;
};
