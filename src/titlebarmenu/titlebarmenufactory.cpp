/*
 * SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>
 * SPDX-FileCopyrightText: 2019 Zain Ahmad <zain.x.ahmad@gmail.com>
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "titlebarmenufactory.h"
#include "decoration.h"
#include "titlebarmenu.h"
#include "titlebarmenuitem.h"

#include <QIcon>
#include <QPainter>

namespace KDecoration3 {

namespace {
    // The library's overflow and search buttons: a theme icon on the default background.
    class IconButton : public TitleBarMenuButton {
    public:
        IconButton(TitleBarMenu* menu, const QString& iconName)
            : TitleBarMenuButton(menu)
            , m_iconName(iconName)
        {
        }

        void paint(QPainter* painter, const QRectF& repaintArea) override
        {
            Q_UNUSED(repaintArea)
            paintBackground(painter);
            const QRectF rect = geometry();
            const qreal side = qMin(rect.width(), rect.height()) * 0.6;
            QRectF iconRect(0, 0, side, side);
            iconRect.moveCenter(rect.center());
            QIcon::fromTheme(m_iconName).paint(painter, iconRect.toAlignedRect(), Qt::AlignCenter, isDimmed() ? QIcon::Disabled : QIcon::Normal);
        }

    private:
        QString m_iconName;
    };
}

TitleBarMenuFactory::TitleBarMenuFactory() = default;

TitleBarMenuFactory::~TitleBarMenuFactory() = default;

TitleBarMenuItem* TitleBarMenuFactory::createItem(TitleBarMenu* menu)
{
    return new TitleBarMenuItem(menu);
}

DecorationButton* TitleBarMenuFactory::createOverflowButton(TitleBarMenu* menu)
{
    return new IconButton(menu, QStringLiteral("application-menu"));
}

DecorationButton* TitleBarMenuFactory::createSearchButton(TitleBarMenu* menu)
{
    return new IconButton(menu, QStringLiteral("edit-find"));
}

}
