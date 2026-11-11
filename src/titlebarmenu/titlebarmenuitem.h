/*
 * SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include "decorationbutton.h"
#include <kdecoration3/titlebarmenu_export.h>

#include <QPointer>

class QAction;

namespace KDecoration3 {
class TitleBarMenu;

/**
 * @brief Base class of the buttons a TitleBarMenu shows.
 *
 * A checkable DecorationButton of type DecorationButtonType::Custom, created with the
 * menu's decoration and the menu as parent.
 */
class KDECORATIONS3TITLEBARMENU_EXPORT TitleBarMenuButton : public DecorationButton {
    Q_OBJECT
public:
    explicit TitleBarMenuButton(TitleBarMenu* menu);
    ~TitleBarMenuButton() override;

    TitleBarMenu* menu() const;

protected:
    /**
     * Whether another button's popup is open, so this one is drawn dimmed.
     */
    bool isDimmed() const;
    /**
     * The default text and icon colour: the window's text colour, dimmed when isDimmed().
     */
    QColor foregroundColor() const;
    /**
     * Paints the default hover and pressed background into geometry().
     */
    void paintBackground(QPainter* painter) const;
};

/**
 * @brief One top-level menu of the window, shown as a label in the titlebar.
 *
 * Themes subclass it to paint the label in their own style.
 */
class KDECORATIONS3TITLEBARMENU_EXPORT TitleBarMenuItem : public TitleBarMenuButton {
    Q_OBJECT
public:
    explicit TitleBarMenuItem(TitleBarMenu* menu);
    ~TitleBarMenuItem() override;

    /**
     * The top-level action this item opens.
     */
    QAction* action() const;
    void setAction(QAction* action);

    /**
     * The action's text without mnemonic markers.
     */
    QString text() const;

    /**
     * The space on each side of the text.
     */
    virtual qreal horizontalPadding() const;
    /**
     * The width this item needs: the text in the title font plus the padding.
     */
    qreal preferredWidth() const;

    void paint(QPainter* painter, const QRectF& repaintArea) override;

private:
    QPointer<QAction> m_action;
};

}
