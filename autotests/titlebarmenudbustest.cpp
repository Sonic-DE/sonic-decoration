/*
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
// Imports a menu from a fake exporter (titlebarmenudbushelper) on a private session bus.
#include "../src/titlebarmenu/titlebarmenumodel.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QMenu>
#include <QProcess>
#include <QSignalSpy>
#include <QTest>

using KDecoration3::TitleBarMenuModel;

class TitleBarMenuDBusTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testImport();
};

static QStringList texts(const QMenu* menu)
{
    QStringList result;
    const auto actions = menu->actions();
    for (auto action : actions) {
        result.append(action->text());
    }
    return result;
}

void TitleBarMenuDBusTest::initTestCase()
{
    QVERIFY(QDBusConnection::sessionBus().isConnected());
}

void TitleBarMenuDBusTest::testImport()
{
    const QString service = QStringLiteral("org.sonicde.TitleBarMenuDBusTest");
    QDBusServiceWatcher watcher(service, QDBusConnection::sessionBus());
    QSignalSpy registered(&watcher, &QDBusServiceWatcher::serviceRegistered);
    QProcess helper;
    helper.setProcessChannelMode(QProcess::ForwardedChannels);
    helper.start(QStringLiteral(HELPER_PATH), {service});
    QVERIFY(registered.wait());

    TitleBarMenuModel model;
    QSignalSpy itemsChanged(&model, &TitleBarMenuModel::itemsChanged);
    model.setMenuAddress(service, QStringLiteral("/MenuBar"));
    QTRY_COMPARE(model.menu() ? texts(model.menu()) : QStringList(), (QStringList{QStringLiteral("File"), QStringLiteral("Edit")}));
    QVERIFY(model.hasLoadedMenu());
    QVERIFY(itemsChanged.count() > 0);

    // A submenu with nothing until AboutToShow loads on demand.
    QMenu* edit = model.menu()->actions().at(1)->menu();
    QVERIFY(edit);
    QVERIFY(edit->actions().isEmpty());
    QSignalSpy submenuReady(&model, &TitleBarMenuModel::submenuReady);
    model.loadSubmenu(edit);
    QTRY_COMPARE(texts(edit), QStringList{QStringLiteral("Copy")});
    QVERIFY(submenuReady.count() > 0);
    QCOMPARE(submenuReady.last().first().value<QMenu*>(), edit);

    // The menu goes when its application leaves the bus.
    itemsChanged.clear();
    helper.kill();
    helper.waitForFinished();
    QVERIFY(itemsChanged.wait());
    QVERIFY(!model.menu());
}

QTEST_MAIN(TitleBarMenuDBusTest)
#include "titlebarmenudbustest.moc"
