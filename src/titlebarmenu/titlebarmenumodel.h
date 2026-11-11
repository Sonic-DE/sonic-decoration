/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#pragma once

#include <kdecoration3/titlebarmenu_export.h>

#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>

class QAction;
class QDBusServiceWatcher;
class QMenu;
class QTimer;

namespace KDecoration3 {
class TitleBarMenuImporter;

/**
 * A window's menu: imported over D-Bus from the application, or an in-process menu.
 *
 * Submenus of an imported menu are empty until loaded; loadSubmenu() loads one, and the
 * caching functions load whole trees a little at a time so searching can see them.
 */
class KDECORATIONS3TITLEBARMENU_EXPORT TitleBarMenuModel : public QObject {
    Q_OBJECT
public:
    explicit TitleBarMenuModel(QObject* parent = nullptr);
    ~TitleBarMenuModel() override;

    /**
     * Imports the menu that @p service exports at @p path. Empty arguments clear the menu.
     */
    void setMenuAddress(const QString& service, const QString& path);
    /**
     * Uses @p menu, an in-process menu this model doesn't own, instead of an imported one.
     */
    void setLocalMenu(QMenu* menu);

    QMenu* menu() const;
    bool hasLoadedMenu() const;
    /**
     * The imported action with the D-Bus menu id @p id, or nullptr.
     */
    QAction* actionForId(int id) const;

    void loadSubmenu(QMenu* menu);
    void cacheSubmenus(QMenu* menu);
    void cacheAll();
    void stopCaching();
    bool isCaching() const;

Q_SIGNALS:
    void itemsChanged();
    void submenuReady(QMenu* menu);
    void searchReady();

private:
    void clear();
    void scheduleItemsChanged();
    void watchTopLevelActions();
    void onMenuUpdated(QMenu* menu);
    void queueSubmenus(QMenu* menu, bool emptyOnly);
    void processNext();
    void finishCachingIfDone();

    QString m_service;
    QString m_path;
    QPointer<TitleBarMenuImporter> m_importer;
    QPointer<QMenu> m_menu;
    bool m_localMenu = false;
    bool m_loaded = false;
    QDBusServiceWatcher* m_serviceWatcher = nullptr;
    QTimer* m_itemsTimer;
    QTimer* m_cacheTimer;
    QList<QPointer<QMenu>> m_cacheQueue;
    QSet<QMenu*> m_queued;
    int m_pendingUpdates = 0;
    bool m_cachingAll = false;
    bool m_caching = false;
};

}
