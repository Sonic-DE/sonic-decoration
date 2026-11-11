/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <QMenu>

namespace KDecoration3 {

/**
 * A menu shown from the titlebar. Left and Right that the menu doesn't use itself ask
 * for the neighbouring titlebar menu.
 */
class TitleBarMenuPopup : public QMenu {
    Q_OBJECT
public:
    explicit TitleBarMenuPopup(QWidget* parent = nullptr);

Q_SIGNALS:
    void previousItemRequested();
    void nextItemRequested();

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

}
