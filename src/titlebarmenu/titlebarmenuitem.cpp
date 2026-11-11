/*
 * SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "titlebarmenuitem.h"
#include "decoratedwindow.h"
#include "decoration.h"
#include "decorationsettings.h"
#include "titlebarmenu.h"

#include <QAction>
#include <QFontMetricsF>
#include <QPainter>
#include <QPalette>

namespace KDecoration3 {

static QString displayText(const QString& text)
{
    // "&File" shows as "File", "&&" as "&".
    QString result;
    result.reserve(text.size());
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('&')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('&')) {
                result.append(QLatin1Char('&'));
                ++i;
            }
            continue;
        }
        result.append(text.at(i));
    }
    return result;
}

TitleBarMenuButton::TitleBarMenuButton(TitleBarMenu* menu)
    : DecorationButton(DecorationButtonType::Custom, menu->decoration(), menu)
{
    setCheckable(true);
}

TitleBarMenuButton::~TitleBarMenuButton() = default;

TitleBarMenu* TitleBarMenuButton::menu() const
{
    return qobject_cast<TitleBarMenu*>(parent());
}

bool TitleBarMenuButton::isDimmed() const
{
    const TitleBarMenu* group = menu();
    return group && group->isPopupOpen() && !isChecked();
}

QColor TitleBarMenuButton::foregroundColor() const
{
    QColor color = decoration()->window()->palette().color(QPalette::WindowText);
    if (isDimmed()) {
        color.setAlphaF(color.alphaF() * 0.6);
    }
    return color;
}

void TitleBarMenuButton::paintBackground(QPainter* painter) const
{
    // While a popup holds the pointer, hover states go stale; only the open item is highlighted.
    if (!(isHovered() && !isDimmed()) && !isPressed() && !isChecked()) {
        return;
    }
    QColor color = decoration()->window()->palette().color(QPalette::Highlight);
    color.setAlphaF(isPressed() || isChecked() ? 0.4 : 0.2);
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    const qreal radius = decoration()->settings()->smallSpacing();
    painter->drawRoundedRect(geometry().adjusted(1, 1, -1, -1), radius, radius);
    painter->restore();
}

TitleBarMenuItem::TitleBarMenuItem(TitleBarMenu* menu)
    : TitleBarMenuButton(menu)
{
}

TitleBarMenuItem::~TitleBarMenuItem() = default;

QAction* TitleBarMenuItem::action() const
{
    return m_action;
}

void TitleBarMenuItem::setAction(QAction* action)
{
    m_action = action;
}

QString TitleBarMenuItem::text() const
{
    return m_action ? displayText(m_action->text()) : QString();
}

qreal TitleBarMenuItem::horizontalPadding() const
{
    return decoration()->settings()->smallSpacing() * 2;
}

qreal TitleBarMenuItem::preferredWidth() const
{
    return QFontMetricsF(decoration()->settings()->font()).horizontalAdvance(text()) + 2 * horizontalPadding();
}

void TitleBarMenuItem::paint(QPainter* painter, const QRectF& repaintArea)
{
    Q_UNUSED(repaintArea)
    paintBackground(painter);
    painter->save();
    painter->setFont(decoration()->settings()->font());
    painter->setPen(foregroundColor());
    painter->drawText(geometry(), Qt::AlignCenter | Qt::TextSingleLine, text());
    painter->restore();
}

}

#include "moc_titlebarmenuitem.cpp"
