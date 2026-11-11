/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2016 Chinmoy Ranjan Pradhan <chinmoyrp65@gmail.com>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "titlebarmenumodel.h"
#include "titlebarmenupopup.h"

#include "dbusmenuimporter.h"

#include <Sonic/Logging/Log>

#include <QAction>
#include <QApplication>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QIcon>
#include <QMenu>
#include <QTimer>

namespace KDecoration3 {

class TitleBarMenuImporter : public DBusMenuImporter {
public:
    TitleBarMenuImporter(const QString& service, const QString& path, QObject* parent)
        : DBusMenuImporter(service, path, parent)
    {
    }

protected:
    QIcon iconForName(const QString& name) override
    {
        return QIcon::fromTheme(name);
    }

    QMenu* createMenu(QWidget* parent) override
    {
        return new TitleBarMenuPopup(parent);
    }
};

TitleBarMenuModel::TitleBarMenuModel(QObject* parent)
    : QObject(parent)
    , m_itemsTimer(new QTimer(this))
    , m_cacheTimer(new QTimer(this))
{
    // Several changes within one event-loop pass make one update.
    m_itemsTimer->setSingleShot(true);
    m_itemsTimer->setInterval(0);
    connect(m_itemsTimer, &QTimer::timeout, this, &TitleBarMenuModel::itemsChanged);

    m_cacheTimer->setSingleShot(true);
    m_cacheTimer->setInterval(10);
    connect(m_cacheTimer, &QTimer::timeout, this, &TitleBarMenuModel::processNext);
}

TitleBarMenuModel::~TitleBarMenuModel() = default;

void TitleBarMenuModel::clear()
{
    stopCaching();
    m_pendingUpdates = 0;
    if (m_importer) {
        m_importer->deleteLater();
    }
    m_importer.clear();
    m_menu.clear();
    m_service.clear();
    m_path.clear();
    m_localMenu = false;
    m_loaded = false;
    if (m_serviceWatcher) {
        m_serviceWatcher->setWatchedServices({});
    }
}

void TitleBarMenuModel::setMenuAddress(const QString& service, const QString& path)
{
    if (service.isEmpty() || path.isEmpty()) {
        if (m_menu || m_importer) {
            clear();
            scheduleItemsChanged();
        }
        return;
    }

    stopCaching();
    m_pendingUpdates = 0;
    if (!m_localMenu && m_importer && service == m_service && path == m_path) {
        QMetaObject::invokeMethod(m_importer, qOverload<>(&DBusMenuImporter::updateMenu), Qt::QueuedConnection);
        return;
    }

    if (!qobject_cast<QApplication*>(QCoreApplication::instance())) {
        sonicDebug() << "Not importing a window menu: the process has no QApplication";
        return;
    }

    clear();
    sonicDebug() << "Importing the window menu at" << service << path;
    m_service = service;
    m_path = path;

    if (!m_serviceWatcher) {
        m_serviceWatcher = new QDBusServiceWatcher(this);
        m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
        m_serviceWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString& serviceName) {
            if (serviceName == m_service) {
                sonicDebug() << "The window menu's service left the bus:" << serviceName;
                clear();
                scheduleItemsChanged();
            }
        });
    }
    m_serviceWatcher->setWatchedServices({service});

    m_importer = new TitleBarMenuImporter(service, path, this);
    m_menu = m_importer->menu();
    connect(m_importer.data(), &DBusMenuImporter::menuUpdated, this, &TitleBarMenuModel::onMenuUpdated);
    QMetaObject::invokeMethod(m_importer, qOverload<>(&DBusMenuImporter::updateMenu), Qt::QueuedConnection);
}

void TitleBarMenuModel::setLocalMenu(QMenu* menu)
{
    if (m_localMenu && m_menu == menu) {
        return;
    }
    clear();
    m_menu = menu;
    m_localMenu = menu != nullptr;
    m_loaded = m_localMenu;
    watchTopLevelActions();
    scheduleItemsChanged();
}

QMenu* TitleBarMenuModel::menu() const
{
    return m_menu;
}

bool TitleBarMenuModel::hasLoadedMenu() const
{
    return m_loaded && m_menu;
}

QAction* TitleBarMenuModel::actionForId(int id) const
{
    return m_importer ? m_importer->actionForId(id) : nullptr;
}

void TitleBarMenuModel::scheduleItemsChanged()
{
    m_itemsTimer->start();
}

void TitleBarMenuModel::watchTopLevelActions()
{
    if (!m_menu) {
        return;
    }
    const auto actions = m_menu->actions();
    for (QAction* action : actions) {
        connect(action, &QAction::changed, this, &TitleBarMenuModel::scheduleItemsChanged, Qt::UniqueConnection);
        connect(action, &QObject::destroyed, this, &TitleBarMenuModel::scheduleItemsChanged, Qt::UniqueConnection);
    }
}

void TitleBarMenuModel::onMenuUpdated(QMenu* menu)
{
    if (menu == m_menu) {
        m_loaded = true;
        watchTopLevelActions();
        scheduleItemsChanged();
        return;
    }

    Q_EMIT submenuReady(menu);
    if (m_caching) {
        queueSubmenus(menu, false);
    }
    if (m_pendingUpdates > 0) {
        --m_pendingUpdates;
    }
    finishCachingIfDone();
}

void TitleBarMenuModel::loadSubmenu(QMenu* menu)
{
    if (!menu) {
        return;
    }
    if (m_localMenu) {
        QMetaObject::invokeMethod(
            this,
            [this, menu = QPointer<QMenu>(menu)] {
                if (menu) {
                    Q_EMIT submenuReady(menu);
                }
            },
            Qt::QueuedConnection);
        return;
    }
    if (m_importer) {
        m_importer->updateMenu(menu);
    }
}

void TitleBarMenuModel::queueSubmenus(QMenu* menu, bool emptyOnly)
{
    const bool wasIdle = m_cacheQueue.isEmpty();
    const auto actions = menu->actions();
    for (QAction* action : actions) {
        QMenu* submenu = action->menu();
        if (!submenu || m_queued.contains(submenu) || (emptyOnly && !submenu->actions().isEmpty())) {
            continue;
        }
        m_queued.insert(submenu);
        m_cacheQueue.append(submenu);
    }
    if (wasIdle && !m_cacheQueue.isEmpty()) {
        processNext();
    }
}

void TitleBarMenuModel::cacheSubmenus(QMenu* menu)
{
    if (!menu || m_localMenu || !m_importer) {
        return;
    }
    if (!m_caching) {
        sonicDebug() << "Caching the submenus of an open window menu";
    }
    m_caching = true;
    queueSubmenus(menu, true);
}

void TitleBarMenuModel::cacheAll()
{
    if (m_localMenu || !m_importer || !m_menu) {
        QMetaObject::invokeMethod(this, &TitleBarMenuModel::searchReady, Qt::QueuedConnection);
        return;
    }
    if (m_cachingAll) {
        return;
    }
    sonicDebug() << "Caching the whole window menu for searching";
    m_caching = true;
    m_cachingAll = true;
    queueSubmenus(m_menu, false);
    finishCachingIfDone();
}

void TitleBarMenuModel::stopCaching()
{
    if (!m_caching) {
        return;
    }
    sonicDebug() << "Stopped caching the window menu";
    m_cacheQueue.clear();
    m_queued.clear();
    m_cacheTimer->stop();
    m_caching = false;
    m_cachingAll = false;
}

bool TitleBarMenuModel::isCaching() const
{
    return m_caching && (!m_cacheQueue.isEmpty() || m_pendingUpdates > 0);
}

void TitleBarMenuModel::processNext()
{
    if (m_cacheQueue.isEmpty()) {
        finishCachingIfDone();
        return;
    }
    QPointer<QMenu> menu = m_cacheQueue.takeFirst();
    m_queued.remove(menu.data());
    if (menu && m_importer) {
        ++m_pendingUpdates;
        m_importer->updateMenu(menu);
    }
    // One submenu per tick keeps the compositor responsive.
    m_cacheTimer->start();
}

void TitleBarMenuModel::finishCachingIfDone()
{
    if (!m_caching || !m_cacheQueue.isEmpty() || m_pendingUpdates > 0) {
        return;
    }
    const bool all = m_cachingAll;
    m_caching = false;
    m_cachingAll = false;
    if (all) {
        Q_EMIT searchReady();
    }
}

}

#include "moc_titlebarmenumodel.cpp"
