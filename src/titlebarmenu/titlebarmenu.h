/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "decorationbuttongroup.h"
#include "titlebarmenufactory.h"
#include <kdecoration3/titlebarmenu_export.h>

#include <QList>

#include <memory>

class QAction;
class QMenu;

namespace KDecoration3 {
class TitleBarMenuItem;
class TitleBarMenuModel;
class TitleBarMenuPrivate;

/**
 * @brief Shows a window's application menu in the titlebar, at the place of the
 * ApplicationMenu button.
 *
 * A theme that supports titlebar menus sets "X-SonicDE-TitleBarMenu": true in its plugin
 * metadata, creates one TitleBarMenu per decoration and anchors it to its ApplicationMenu
 * button with setAnchor(). The anchor is disabled and stays in its group as a placeholder:
 * the theme gives it the menu's width, calls updateItemsGeometry() and setPos() from its
 * layout code, paints the menu and keeps the caption clear of it.
 *
 * The menu follows DecorationSettings::applicationMenuStyle() and
 * DecorationSettings::isApplicationMenuSearchEnabled().
 */
class KDECORATIONS3TITLEBARMENU_EXPORT TitleBarMenu : public DecorationButtonGroup {
    Q_OBJECT
public:
    TitleBarMenu(Decoration* decoration, std::unique_ptr<TitleBarMenuFactory> factory);
    ~TitleBarMenu() override;

    /**
     * Sets the ApplicationMenu button the menu replaces. It's disabled while anchored.
     * Without an anchor, or while it's hidden, the menu has no buttons.
     */
    void setAnchor(DecorationButton* anchor);
    DecorationButton* anchor() const;

    /**
     * Sizes the buttons and hides the items that don't fit in @p availableRect, which starts
     * at the anchor's place. The overflow and search buttons get @p iconButtonSize. A null
     * rectangle hides everything.
     */
    void updateItemsGeometry(const QRectF& availableRect, const QSizeF& iconButtonSize);
    /**
     * The width of the visible buttons after the last updateItemsGeometry().
     */
    qreal visibleItemsWidth() const;

    /**
     * The menu's opacity: below 1 only in ApplicationMenuStyle::ShownOnHover.
     */
    qreal opacity() const;
    /**
     * Whether the pointer is over the titlebar, for ApplicationMenuStyle::ShownOnHover.
     */
    void setHovered(bool hovered);
    bool isHovered() const;
    void setAnimationsEnabled(bool enabled);
    void setAnimationDuration(int milliseconds);

    bool isPopupOpen() const;
    /**
     * The index in buttons() of the button whose popup is open, or -1.
     */
    int openItemIndex() const;

    /**
     * Opens the popup of the button at @p index in buttons().
     */
    void openItem(int index);
    void openOverflow();
    void openSearch();
    void openPreviousItem();
    void openNextItem();
    /**
     * While a popup is open, opens the button under @p pos (decoration coordinates).
     */
    void updateHoveredItem(const QPointF& pos);
    /**
     * Clears every button's pressed state, for when a window move starts on the menu.
     */
    void releaseAllItems();

    /**
     * The actions whose path ("File » Save As…") contains @p text, case-insensitively,
     * leaving out separators and disabled actions. Empty below three characters.
     */
    QList<QAction*> collectSearchResults(const QString& text) const;
    /**
     * Shows the results for @p text in the search popup.
     */
    void updateSearchResults(const QString& text);
    QMenu* searchPopup() const;

    TitleBarMenuModel* model() const;
    QList<TitleBarMenuItem*> items() const;
    DecorationButton* overflowButton() const;
    DecorationButton* searchButton() const;

    void paint(QPainter* painter, const QRectF& repaintArea) override;

Q_SIGNALS:
    void itemsChanged();
    void visibleItemsWidthChanged();
    void opacityChanged(qreal opacity);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    friend class TitleBarMenuPrivate;
    std::unique_ptr<TitleBarMenuPrivate> d;
};

}
