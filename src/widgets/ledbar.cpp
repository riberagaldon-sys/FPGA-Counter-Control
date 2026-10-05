#include "ledbar.h"

#include <QPainter>
#include <QPaintEvent>

LedBar::LedBar(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(105);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void LedBar::setMask(quint16 mask)
{
    mask &= 0x0FFFU;
    if (m_mask == mask)
        return;
    m_mask = mask;
    update();
}

QSize LedBar::sizeHint() const
{
    return QSize(1180, 120);
}

QSize LedBar::minimumSizeHint() const
{
    return QSize(820, 96);
}

void LedBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outer = rect().adjusted(8, 6, -8, -6);
    painter.setPen(QPen(QColor("#CBD5E1"), 1));
    painter.setBrush(QColor("#F8FAFC"));
    painter.drawRoundedRect(outer, 12, 12);

    const qreal gap = 9.0;
    const qreal itemWidth = (outer.width() - 26.0 - gap * 11.0) / 12.0;
    const qreal lampSize = qMin(itemWidth * .62, outer.height() * .48);
    QFont labelFont = painter.font();
    labelFont.setPointSizeF(qMax(9.0, itemWidth * .12));
    labelFont.setBold(true);
    painter.setFont(labelFont);

    for (int i = 0; i < 12; ++i) {
        const qreal x = outer.left() + 13.0 + i * (itemWidth + gap);
        const QPointF center(x + itemWidth / 2.0, outer.top() + outer.height() * .42);
        const bool on = (m_mask & (1U << i)) != 0;

        if (on) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(34, 197, 94, 65));
            painter.drawEllipse(center, lampSize * .68, lampSize * .68);
        }

        painter.setPen(QPen(on ? QColor("#15803D") : QColor("#64748B"), 2));
        painter.setBrush(on ? QColor("#31D158") : QColor("#D8DEE8"));
        painter.drawEllipse(center, lampSize / 2.0, lampSize / 2.0);

        painter.setPen(QColor("#334155"));
        const QRectF labelRect(x, outer.bottom() - 31.0, itemWidth, 24.0);
        painter.drawText(labelRect, Qt::AlignCenter, QStringLiteral("LED%1").arg(i + 1));
    }
}
