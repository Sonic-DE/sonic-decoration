/*
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../src/titlebarmenu/titlebarmenu.h"
#include "../src/decorationsettings.h"
#include "../src/titlebarmenu/titlebarmenuitem.h"
#include "../src/titlebarmenu/titlebarmenumodel.h"
#include "mockbridge.h"
#include "mockbutton.h"
#include "mockdecoration.h"
#include "mocksettings.h"
#include "mockwindow.h"

#include <QAction>
#include <QMenu>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QTest>
#include <QWidgetAction>

using KDecoration3::ApplicationMenuStyle;
using KDecoration3::DecorationButton;
using KDecoration3::TitleBarMenu;

namespace {
QMenu* popupOf(const QSignalSpy& spy, qsizetype call)
{
    return static_cast<QMenu*>(spy.at(call).first().value<void*>());
}

const QSizeF s_iconSize(20, 20);
const QRectF s_wideRect(0, 0, 10000, 20);

// A decoration with an anchored titlebar menu.
struct Fixture {
    Fixture()
        : settings(std::make_shared<KDecoration3::DecorationSettings>(&bridge))
        , decoration(&bridge)
    {
        decoration.setSettings(settings);
        mockSettings = bridge.lastCreatedSettings();
        window = bridge.lastCreatedWindow();
        anchor = new MockButton(KDecoration3::DecorationButtonType::ApplicationMenu, &decoration, &decoration);
        menu = new TitleBarMenu(&decoration, nullptr);
        menu->setAnimationsEnabled(false);
        menu->setAnchor(anchor);
        menu->updateItemsGeometry(s_wideRect, s_iconSize);
    }

    // Uses a fixture menu: an unlabelled item (as GTK has), File, Edit and View.
    bool useFixtureMenu()
    {
        unlabelled = fixtureMenu.addMenu(QString());
        unlabelled->addAction(QStringLiteral("Hidden"));
        QMenu* file = fixtureMenu.addMenu(QStringLiteral("&File"));
        file->addAction(QStringLiteral("&Open"));
        file->addAction(QStringLiteral("Save &As…"));
        file->addSeparator();
        file->addAction(QStringLiteral("&Quit"));
        QMenu* edit = fixtureMenu.addMenu(QStringLiteral("&Edit"));
        edit->addAction(QStringLiteral("Undo"))->setEnabled(false);
        QMenu* find = edit->addMenu(QStringLiteral("Find"));
        find->addAction(QStringLiteral("Find Next"));
        find->menuAction()->setEnabled(false);
        QMenu* view = fixtureMenu.addMenu(QStringLiteral("&View"));
        view->addAction(QStringLiteral("Zoom In"));

        QSignalSpy itemsChanged(menu, &TitleBarMenu::itemsChanged);
        menu->model()->setLocalMenu(&fixtureMenu);
        return itemsChanged.wait();
    }

    int indexOf(DecorationButton* button) const
    {
        return menu->buttons().indexOf(button);
    }

    int itemIndex(const QString& text) const
    {
        const auto items = menu->items();
        for (auto item : items) {
            if (item->text() == text) {
                return indexOf(item);
            }
        }
        return -1;
    }

    MockBridge bridge;
    std::shared_ptr<KDecoration3::DecorationSettings> settings;
    MockDecoration decoration;
    MockSettings* mockSettings = nullptr;
    MockWindow* window = nullptr;
    MockButton* anchor = nullptr;
    TitleBarMenu* menu = nullptr;
    QMenu fixtureMenu;
    QMenu* unlabelled = nullptr;
};
}

class TitleBarMenuTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testAlwaysShownItems();
    void testOverflow();
    void testSingleButton();
    void testShownOnHover();
    void testSearchButtonSetting_data();
    void testSearchButtonSetting();
    void testSearchResults();
    void testSearchResultsDoNotPileUp();
    void testSearchKeyboard();
    void testNavigation();
    void testNoButtons();
    void testPreviewSample();
    void testAnchor();
    void testPointerMotionWhilePopupOpen();
    void testApplicationMenuHandler();
    void testVisibleWidthChangedOnlyOnChange();
};

void TitleBarMenuTest::testAlwaysShownItems()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);

    const auto actions = f.fixtureMenu.actions();
    const auto items = f.menu->items();
    QCOMPARE(items.size(), actions.size());
    for (int i = 0; i < items.size(); ++i) {
        QCOMPARE(items.at(i)->action(), actions.at(i));
        QCOMPARE(items.at(i)->isVisible(), !actions.at(i)->text().isEmpty());
    }
    QVERIFY(!f.menu->overflowButton()->isVisible());
    QVERIFY(f.menu->searchButton()->isVisible());
}

void TitleBarMenuTest::testOverflow()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QVERIFY(f.useFixtureMenu());

    // Room for the first labelled item, the overflow and the search button only.
    const auto items = f.menu->items();
    const qreal firstWidth = items.at(1)->preferredWidth();
    f.menu->updateItemsGeometry(QRectF(0, 0, firstWidth + 2 * s_iconSize.width(), 20), s_iconSize);

    QList<QAction*> hidden;
    for (auto item : items) {
        if (!item->text().isEmpty() && !item->isVisible()) {
            hidden.append(item->action());
        }
    }
    QVERIFY(!hidden.isEmpty());
    QVERIFY(items.at(1)->isVisible());
    QVERIFY(f.menu->overflowButton()->isVisible());

    QSignalSpy popupRequested(f.window, &MockWindow::popupRequested);
    f.menu->openOverflow();
    QCOMPARE(popupRequested.count(), 1);
    QCOMPARE(popupOf(popupRequested, 0)->actions(), hidden);

    // With room for everything, nothing overflows.
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);
    QVERIFY(!f.menu->overflowButton()->isVisible());
}

void TitleBarMenuTest::testSingleButton()
{
    Fixture f;
    QCOMPARE(f.mockSettings->applicationMenuStyle(), ApplicationMenuStyle::SingleButton);
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);

    QVERIFY(f.menu->items().isEmpty());
    QVERIFY(f.menu->overflowButton()->isVisible());
    QVERIFY(f.menu->searchButton()->isVisible());
    QCOMPARE(f.menu->buttons().size(), 2);

    QSignalSpy popupRequested(f.window, &MockWindow::popupRequested);
    f.menu->openOverflow();
    QCOMPARE(popupRequested.count(), 1);
    QCOMPARE(popupOf(popupRequested, 0)->actions(), f.fixtureMenu.actions());
}

void TitleBarMenuTest::testShownOnHover()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::ShownOnHover);
    QVERIFY(f.useFixtureMenu());

    QCOMPARE(f.menu->opacity(), 0.0);
    f.menu->setHovered(true);
    QCOMPARE(f.menu->opacity(), 1.0);
    f.menu->setHovered(false);
    QCOMPARE(f.menu->opacity(), 0.0);

    f.menu->openItem(f.itemIndex(QStringLiteral("File")));
    QVERIFY(f.menu->isPopupOpen());
    QCOMPARE(f.menu->opacity(), 1.0);

    // Other styles are always opaque.
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QCOMPARE(f.menu->opacity(), 1.0);
}

void TitleBarMenuTest::testSearchButtonSetting_data()
{
    QTest::addColumn<ApplicationMenuStyle>("style");
    QTest::newRow("SingleButton") << ApplicationMenuStyle::SingleButton;
    QTest::newRow("AlwaysShown") << ApplicationMenuStyle::AlwaysShown;
    QTest::newRow("ShownOnHover") << ApplicationMenuStyle::ShownOnHover;
}

void TitleBarMenuTest::testSearchButtonSetting()
{
    QFETCH(ApplicationMenuStyle, style);
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(style);
    QVERIFY(f.useFixtureMenu());
    QVERIFY(f.menu->searchButton());
    QCOMPARE(f.menu->buttons().last(), f.menu->searchButton());

    f.mockSettings->setApplicationMenuSearchEnabled(false);
    QVERIFY(!f.menu->searchButton());
    QVERIFY(f.menu->overflowButton());

    f.mockSettings->setApplicationMenuSearchEnabled(true);
    QVERIFY(f.menu->searchButton());
}

void TitleBarMenuTest::testSearchResults()
{
    Fixture f;
    QVERIFY(f.useFixtureMenu());

    auto texts = [&f](const QString& text) {
        QStringList result;
        const auto actions = f.menu->collectSearchResults(text);
        for (auto action : actions) {
            result.append(action->text());
        }
        return result;
    };

    QCOMPARE(texts(QStringLiteral("ope")), QStringList{QStringLiteral("&Open")});
    QCOMPARE(texts(QStringLiteral("OPE")), QStringList{QStringLiteral("&Open")});
    // by path, with mnemonics removed
    QCOMPARE(texts(QStringLiteral("file » save as")), QStringList{QStringLiteral("Save &As…")});
    // three characters needed
    QVERIFY(texts(QStringLiteral("op")).isEmpty());
    // disabled actions, and actions under a disabled submenu, are left out
    QVERIFY(texts(QStringLiteral("undo")).isEmpty());
    QVERIFY(texts(QStringLiteral("find next")).isEmpty());
    // separators are never results
    const auto all = f.menu->collectSearchResults(QStringLiteral(" » "));
    for (auto action : all) {
        QVERIFY(!action->isSeparator());
    }
}

void TitleBarMenuTest::testSearchResultsDoNotPileUp()
{
    Fixture f;
    QVERIFY(f.useFixtureMenu());

    f.menu->updateSearchResults(QStringLiteral("ope"));
    const int count = f.menu->searchPopup()->actions().size();
    QCOMPARE(count, 3); // the search field, a separator and one result
    f.menu->updateSearchResults(QStringLiteral("ope"));
    QCOMPARE(f.menu->searchPopup()->actions().size(), count);
    f.menu->updateSearchResults(QStringLiteral("zoom"));
    QCOMPARE(f.menu->searchPopup()->actions().size(), count);
    f.menu->updateSearchResults(QString());
    QCOMPARE(f.menu->searchPopup()->actions().size(), 2);
}

void TitleBarMenuTest::testSearchKeyboard()
{
    Fixture f;
    QVERIFY(f.useFixtureMenu());
    f.menu->openSearch();
    QMenu* popup = f.menu->searchPopup();
    QVERIFY(popup);
    auto field = qobject_cast<QWidgetAction*>(popup->actions().first())->defaultWidget();
    QVERIFY(field);

    f.menu->updateSearchResults(QStringLiteral("zoo")); // matches nothing
    f.menu->updateSearchResults(QStringLiteral("save"));
    QAction* original = f.fixtureMenu.actions().at(1)->menu()->actions().at(1); // File » Save As…
    QSignalSpy triggered(original, &QAction::triggered);

    // The search field keeps the focus: Down picks the first result, Return triggers it.
    QTest::keyClick(field, Qt::Key_Down);
    QCOMPARE(popup->activeAction(), popup->actions().at(2));
    QTest::keyClick(field, Qt::Key_Return);
    QCOMPARE(triggered.count(), 1);
}

void TitleBarMenuTest::testNavigation()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);

    const int file = f.itemIndex(QStringLiteral("File"));
    const int edit = f.itemIndex(QStringLiteral("Edit"));
    const int view = f.itemIndex(QStringLiteral("View"));
    const int search = f.indexOf(f.menu->searchButton());

    f.menu->openItem(file);
    QCOMPARE(f.menu->openItemIndex(), file);
    f.menu->openNextItem();
    QCOMPARE(f.menu->openItemIndex(), edit);
    f.menu->openNextItem();
    QCOMPARE(f.menu->openItemIndex(), view);
    // the hidden overflow button is skipped
    f.menu->openNextItem();
    QCOMPARE(f.menu->openItemIndex(), search);
    // wraps past the hidden, unlabelled item
    f.menu->openNextItem();
    QCOMPARE(f.menu->openItemIndex(), file);
    f.menu->openPreviousItem();
    QCOMPARE(f.menu->openItemIndex(), search);
}

void TitleBarMenuTest::testNoButtons()
{
    Fixture f;
    QVERIFY(f.useFixtureMenu());
    QVERIFY(!f.menu->buttons().isEmpty());

    f.window->setModal(true);
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown); // makes it update
    QVERIFY(f.menu->buttons().isEmpty());
    f.window->setModal(false);
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::SingleButton);
    QVERIFY(!f.menu->buttons().isEmpty());

    f.window->setHasApplicationMenu(false);
    QTRY_VERIFY(f.menu->buttons().isEmpty());
    QCOMPARE(f.menu->visibleItemsWidth(), 0.0);
}

void TitleBarMenuTest::testPreviewSample()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    // A menu without an address is only reported by the settings preview.
    QVERIFY(f.window->applicationMenuServiceName().isEmpty());
    QTRY_COMPARE(f.menu->items().size(), 4);
    QStringList texts;
    for (auto item : f.menu->items()) {
        texts.append(item->text());
    }
    QCOMPARE(texts, (QStringList{QStringLiteral("File"), QStringLiteral("Edit"), QStringLiteral("View"), QStringLiteral("Help")}));
}

void TitleBarMenuTest::testAnchor()
{
    Fixture f;
    QVERIFY(!f.anchor->isEnabled());

    MockButton other(KDecoration3::DecorationButtonType::ApplicationMenu, &f.decoration);
    f.menu->setAnchor(&other);
    QVERIFY(f.anchor->isEnabled());
    QVERIFY(!other.isEnabled());

    // A hidden anchor shows no menu.
    QVERIFY(!f.menu->buttons().isEmpty());
    other.setVisible(false);
    QVERIFY(f.menu->buttons().isEmpty());

    f.menu->setAnchor(nullptr);
    QVERIFY(other.isEnabled());
    QVERIFY(f.menu->buttons().isEmpty());
}

void TitleBarMenuTest::testPointerMotionWhilePopupOpen()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);
    f.menu->setPos(QPointF(10, 0));

    const QPointF windowPosition(100, 50);
    f.window->setPosition(windowPosition);
    const int file = f.itemIndex(QStringLiteral("File"));
    const int edit = f.itemIndex(QStringLiteral("Edit"));

    QSignalSpy popupRequested(f.window, &MockWindow::popupRequested);
    f.menu->openItem(file);
    QCOMPARE(popupRequested.count(), 1);
    QMenu* popup = popupOf(popupRequested, popupRequested.count() - 1);

    const QPointF global = windowPosition + f.menu->buttons().at(edit)->geometry().center();
    QMouseEvent move(QEvent::MouseMove, QPointF(1, 1), global, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(popup, &move);
    QCOMPARE(f.menu->openItemIndex(), edit);
}

void TitleBarMenuTest::testApplicationMenuHandler()
{
    Fixture f;
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);

    QSignalSpy popupRequested(f.window, &MockWindow::popupRequested);
    QSignalSpy hostRequested(f.window, &MockWindow::applicationMenuRequested);
    f.decoration.showApplicationMenu(0);
    QCOMPARE(popupRequested.count(), 1);
    QCOMPARE(hostRequested.count(), 0);
    QVERIFY(f.menu->isPopupOpen());

    // Without an anchor the host shows the menu, as before.
    f.menu->setAnchor(nullptr);
    f.decoration.showApplicationMenu(0);
    QCOMPARE(hostRequested.count(), 1);
}

void TitleBarMenuTest::testVisibleWidthChangedOnlyOnChange()
{
    Fixture f;
    f.mockSettings->setApplicationMenuStyle(ApplicationMenuStyle::AlwaysShown);
    QVERIFY(f.useFixtureMenu());
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);

    QSignalSpy widthChanged(f.menu, &TitleBarMenu::visibleItemsWidthChanged);
    f.menu->updateItemsGeometry(s_wideRect, s_iconSize);
    QCOMPARE(widthChanged.count(), 0);
    f.menu->updateItemsGeometry(QRectF(0, 0, 1, 20), s_iconSize);
    QCOMPARE(widthChanged.count(), 1);
    QVERIFY(f.menu->visibleItemsWidth() > 0); // overflow and search remain
}

QTEST_MAIN(TitleBarMenuTest)
#include "titlebarmenutest.moc"
