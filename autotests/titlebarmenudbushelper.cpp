/*
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
// A fake application exporting a menu over com.canonical.dbusmenu, for titleBarMenuDBusTest.
// "File" has its items from the start; "Edit" gets its item only after AboutToShow.
#include "dbusmenutypes_p.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusVariant>

#include <cstdio>

class FakeMenuExporter : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.canonical.dbusmenu")
public:
    using QObject::QObject;

public Q_SLOTS:
    uint GetLayout(int parentId, int recursionDepth, const QStringList& propertyNames, DBusMenuLayoutItem& item)
    {
        Q_UNUSED(recursionDepth)
        Q_UNUSED(propertyNames)
        item = layoutItem(parentId);
        if (parentId == 0) {
            item.children = {layoutItem(1), layoutItem(2)};
        } else if (parentId == 1) {
            item.children = {layoutItem(10), layoutItem(11)};
        } else if (parentId == 2 && m_editLoaded) {
            item.children = {layoutItem(20)};
        }
        return m_revision;
    }

    DBusMenuItemList GetGroupProperties(const QList<int>& ids, const QStringList& propertyNames)
    {
        Q_UNUSED(propertyNames)
        DBusMenuItemList result;
        for (int id : ids) {
            result.append({id, layoutItem(id).properties});
        }
        return result;
    }

    QDBusVariant GetProperty(int id, const QString& property)
    {
        return QDBusVariant(layoutItem(id).properties.value(property));
    }

    void Event(int id, const QString& eventId, const QDBusVariant& data, uint timestamp)
    {
        Q_UNUSED(id)
        Q_UNUSED(eventId)
        Q_UNUSED(data)
        Q_UNUSED(timestamp)
    }

    bool AboutToShow(int id)
    {
        if (id == 2 && !m_editLoaded) {
            m_editLoaded = true;
            ++m_revision;
            return true;
        }
        return false;
    }

Q_SIGNALS:
    void LayoutUpdated(uint revision, int parentId);

private:
    static DBusMenuLayoutItem layoutItem(int id)
    {
        static const QHash<int, QString> labels{{1, QStringLiteral("File")},
            {2, QStringLiteral("Edit")},
            {10, QStringLiteral("Open")},
            {11, QStringLiteral("Quit")},
            {20, QStringLiteral("Copy")}};
        DBusMenuLayoutItem item;
        item.id = id;
        if (id != 0) {
            item.properties.insert(QStringLiteral("label"), labels.value(id));
        }
        if (id == 0 || id == 1 || id == 2) {
            item.properties.insert(QStringLiteral("children-display"), QStringLiteral("submenu"));
        }
        return item;
    }

    uint m_revision = 1;
    bool m_editLoaded = false;
};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s SERVICE\n", argv[0]);
        return 2;
    }
    DBusMenuTypes_register();

    FakeMenuExporter exporter;
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(QStringLiteral("/MenuBar"), &exporter, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals)) {
        std::fprintf(stderr, "registerObject failed\n");
        return 1;
    }
    if (!bus.registerService(QString::fromLocal8Bit(argv[1]))) {
        std::fprintf(stderr, "registerService failed\n");
        return 1;
    }
    return app.exec();
}

#include "titlebarmenudbushelper.moc"
