/*
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#pragma once

#include <kdecoration3/titlebarmenu_export.h>

namespace KDecoration3 {
class DecorationButton;
class TitleBarMenu;
class TitleBarMenuItem;

/**
 * @brief Creates the buttons of a TitleBarMenu, so a theme can draw them in its own style.
 *
 * Every button must be of type DecorationButtonType::Custom, created with
 * menu->decoration() and with @p menu as its parent. The menu makes it checkable and
 * connects its clicked() signal.
 */
class KDECORATIONS3TITLEBARMENU_EXPORT TitleBarMenuFactory {
public:
    TitleBarMenuFactory();
    virtual ~TitleBarMenuFactory();

    /**
     * One top-level menu's label. The default draws plain text.
     */
    virtual TitleBarMenuItem* createItem(TitleBarMenu* menu);
    /**
     * The button that lists the menus without room, or every menu in
     * ApplicationMenuStyle::SingleButton. The default draws the "application-menu" icon.
     */
    virtual DecorationButton* createOverflowButton(TitleBarMenu* menu);
    /**
     * The button that opens the menu search. The default draws the "edit-find" icon.
     */
    virtual DecorationButton* createSearchButton(TitleBarMenu* menu);
};

}
