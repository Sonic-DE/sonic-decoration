/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "titlebarmenupopup.h"

#include <QGuiApplication>
#include <QKeyEvent>

namespace KDecoration3 {

TitleBarMenuPopup::TitleBarMenuPopup(QWidget* parent)
    : QMenu(parent)
{
}

void TitleBarMenuPopup::keyPressEvent(QKeyEvent* event)
{
    QMenu::keyPressEvent(event);
    if (event->isAccepted() || event->modifiers() != Qt::NoModifier) {
        return;
    }

    const bool rightToLeft = QGuiApplication::layoutDirection() == Qt::RightToLeft;
    const int previousKey = rightToLeft ? Qt::Key_Right : Qt::Key_Left;
    const int nextKey = rightToLeft ? Qt::Key_Left : Qt::Key_Right;
    if (event->key() != previousKey && event->key() != nextKey) {
        return;
    }
    // Left/Right belong to an open submenu.
    if (activeAction() && activeAction()->menu() && activeAction()->menu()->isVisible()) {
        return;
    }

    if (event->key() == previousKey) {
        Q_EMIT previousItemRequested();
    } else {
        Q_EMIT nextItemRequested();
    }
    event->accept();
}

}

#include "moc_titlebarmenupopup.cpp"
