#include "sevensegmentdisplay.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

SevenSegmentDisplay::SevenSegmentDisplay(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(280);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void SevenSegmentDisplay::setValue(quint32 value)
{
    if (m_value == value)
        return;
    m_value = value;
    update();
}

QSize SevenSegmentDisplay::sizeHint() const
{
    return QSize(1180, 320);
}

QSize SevenSegmentDisplay::minimumSizeHint() const
{
    return QSize(840, 255);
}

quint8 SevenSegmentDisplay::segmentMask(int digit)
{
    static constexpr quint8 masks[16] = {
        0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
        0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71
    };
    return masks[digit & 0x0F];
}

void SevenSegmentDisplay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outer = rect().adjusted(8, 8, -8, -8);
    painter.setPen(QPen(QColor("#334155"), 2));
    painter.setBrush(QColor("#0A0D12"));
    painter.drawRoundedRect(outer, 16, 16);

    constexpr int digitCount = 8;
    const qreal gap = qMax<qreal>(8.0, outer.width() * 0.008);
    const qreal side = qMax<qreal>(12.0, outer.width() * 0.012);
    const qreal digitWidth = (outer.width() - side * 2.0 - gap * 7.0) / digitCount;
    const qreal top = outer.top() + 12.0;
    const qreal height = outer.height() - 24.0;

    for (int i = 0; i < digitCount; ++i) {
        const qreal x = outer.left() + side + i * (digitWidth + gap);
        const QRectF digitRect(x, top, digitWidth, height);
        const int shift = (digitCount - 1 - i) * 4;
        paintDigit(painter, digitRect, static_cast<int>((m_value >> shift) & 0x0F));
    }
}

void SevenSegmentDisplay::paintDigit(QPainter &painter, const QRectF &r, int digit) const
{
    painter.setPen(QPen(QColor("#3F4A5A"), 1.2));
    painter.setBrush(QColor("#151A22"));
    painter.drawRoundedRect(r, 8, 8);

    const auto point = [&](qreal x, qreal y) {
        return QPointF(r.left() + x * r.width(), r.top() + y * r.height());
    };

    const QPolygonF segments[7] = {
        {point(.20, .07), point(.80, .07), point(.70, .15), point(.30, .15)},
        {point(.82, .09), point(.91, .18), point(.88, .44), point(.79, .48), point(.73, .41), point(.75, .19)},
        {point(.79, .52), point(.88, .56), point(.91, .82), point(.82, .91), point(.75, .81), point(.73, .59)},
        {point(.30, .85), point(.70, .85), point(.80, .93), point(.20, .93)},
        {point(.18, .52), point(.27, .59), point(.25, .81), point(.18, .91), point(.09, .82), point(.12, .56)},
        {point(.18, .09), point(.25, .19), point(.27, .41), point(.21, .48), point(.12, .44), point(.09, .18)},
        {point(.28, .46), point(.72, .46), point(.79, .50), point(.72, .54), point(.28, .54), point(.21, .50)}
    };

    const quint8 mask = segmentMask(digit);
    for (int segment = 0; segment < 7; ++segment) {
        const bool on = (mask & (1U << segment)) != 0;
        painter.setPen(Qt::NoPen);
        painter.setBrush(on ? QColor("#FF2028") : QColor("#3A1519"));
        painter.drawPolygon(segments[segment]);
    }

    painter.setBrush(QColor("#3A1519"));
    painter.drawEllipse(point(.91, .91), r.width() * .035, r.width() * .035);
}
