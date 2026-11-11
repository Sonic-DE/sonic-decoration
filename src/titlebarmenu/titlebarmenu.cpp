/*
 * SPDX-FileCopyrightText: 2025 Guido Iodice <guido.iodice@gmail.com>
 * SPDX-FileCopyrightText: 2020 Chris Holland <zrenfire@gmail.com>
 * SPDX-FileCopyrightText: 2016 Kai Uwe Broulik <kde@privat.broulik.de>
 * SPDX-FileCopyrightText: 2014 Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * SPDX-FileCopyrightText: 2026 Joseph Crowell <joseph.w.crowell@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "titlebarmenu.h"
#include "decoratedwindow.h"
#include "decoration.h"
#include "decorationsettings.h"
#include "titlebarmenufactory.h"
#include "titlebarmenuitem.h"
#include "titlebarmenumodel.h"
#include "titlebarmenupopup.h"

#include <KLocalizedString>

#include <Sonic/Logging/Log>
#include <Sonic/Logging/Logging>

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QSet>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidgetAction>

SONIC_LOGGING_COMPONENT("org.sonicde.decoration.titlebarmenu")
SONIC_LOGGING_AUTO_INSTALL()

namespace KDecoration3 {

class TitleBarMenuPrivate {
public:
    TitleBarMenuPrivate(TitleBarMenu* q, std::unique_ptr<TitleBarMenuFactory> factory);

    bool widgetsAvailable() const;
    ApplicationMenuStyle style() const;
    void onAddressChanged();
    void applyAddress();
    void updateItems();
    void removeButtons();
    void updateHandler();
    void handleShowRequest(int actionId);
    void updateShowing();
    void setOpacity(qreal opacity);
    void setVisibleWidth(qreal width);
    void showPopup(QMenu* menu, int index);
    void popupAboutToHide(QMenu* menu);
    int nextVisibleItemIndex(int from, bool forward) const;
    void ensureSearchPopup();
    void repositionSearchPopup();
    void clearSearchResults();
    void collectSearchResults(QMenu* menu, const QString& text, QList<QAction*>& results, QSet<QMenu*>& visited) const;

    struct ActionInfo {
        QString path;
        bool enabled;
    };
    static ActionInfo actionPath(QAction* action);

    TitleBarMenu* q;
    QPointer<Decoration> decoration;
    std::unique_ptr<TitleBarMenuFactory> factory;
    TitleBarMenuModel* model;
    QPointer<DecorationButton> anchor;
    QMetaObject::Connection anchorVisibility;
    QList<QPointer<TitleBarMenuItem>> items;
    QPointer<DecorationButton> overflowButton;
    QPointer<DecorationButton> searchButton;

    QPointer<QMenu> openPopup;
    int openIndex = -1;
    int waitingIndex = -1;
    QPointer<DecorationButton> hoveredButton;

    QTimer addressTimer;
    bool addressPending = false;
    QTimer cacheTimer;
    QPointer<QMenu> menuToCache;
    int menuToCacheIndex = -1;
    std::unique_ptr<QMenu> sampleMenu;
    bool handlerSet = false;
    bool warnedNoWidgets = false;

    QPointer<TitleBarMenuPopup> searchPopup;
    QPointer<QLineEdit> searchLineEdit;
    QTimer searchTimer;
    QList<QPointer<QAction>> searchResultActions;
    QString searchText;
    bool searchPending = false;

    QVariantAnimation* animation;
    qreal opacity = 1;
    bool hovered = false;
    bool animationsEnabled = true;

    QRectF availableRect;
    QSizeF iconButtonSize;
    qreal visibleWidth = 0;
};

TitleBarMenuPrivate::TitleBarMenuPrivate(TitleBarMenu* q, std::unique_ptr<TitleBarMenuFactory> factory)
    : q(q)
    , decoration(q->decoration())
    , factory(factory ? std::move(factory) : std::make_unique<TitleBarMenuFactory>())
    , model(new TitleBarMenuModel(q))
    , animation(new QVariantAnimation(q))
{
}

bool TitleBarMenuPrivate::widgetsAvailable() const
{
    return qobject_cast<QApplication*>(QCoreApplication::instance()) != nullptr;
}

ApplicationMenuStyle TitleBarMenuPrivate::style() const
{
    return decoration->settings()->applicationMenuStyle();
}

void TitleBarMenuPrivate::onAddressChanged()
{
    // At most one update per 100 ms; the last change always applies.
    if (addressTimer.isActive()) {
        addressPending = true;
        return;
    }
    applyAddress();
    addressTimer.start();
}

void TitleBarMenuPrivate::applyAddress()
{
    const auto window = decoration->window();
    if (!window->hasApplicationMenu()) {
        model->setMenuAddress(QString(), QString());
        updateItems();
        return;
    }
    const QString service = window->applicationMenuServiceName();
    const QString path = window->applicationMenuObjectPath();
    if (service.isEmpty() || path.isEmpty()) {
        // Only the settings preview reports a menu without an address: show a sample.
        if (!sampleMenu && widgetsAvailable()) {
            sampleMenu = std::make_unique<QMenu>();
            sampleMenu->addMenu(i18nc("@title:menu", "File"));
            sampleMenu->addMenu(i18nc("@title:menu", "Edit"));
            sampleMenu->addMenu(i18nc("@title:menu", "View"));
            sampleMenu->addMenu(i18nc("@title:menu", "Help"));
        }
        model->setLocalMenu(sampleMenu.get());
        return;
    }
    model->setMenuAddress(service, path);
}

void TitleBarMenuPrivate::removeButtons()
{
    if (openPopup) {
        openPopup->close();
    }
    openIndex = -1;
    waitingIndex = -1;
    hoveredButton.clear();
    const auto old = q->buttons();
    for (DecorationButton* button : old) {
        q->removeButton(button);
    }
    qDeleteAll(old);
    items.clear();
    overflowButton.clear();
    searchButton.clear();
}

void TitleBarMenuPrivate::updateItems()
{
    removeButtons();

    const auto window = decoration->window();
    QMenu* menu = model->menu();
    bool show = anchor && anchor->isVisible() && !window->isModal() && window->hasApplicationMenu() && menu;
    if (show && !widgetsAvailable()) {
        if (!warnedNoWidgets) {
            sonicDebug() << "Not showing the window menu: the process has no QApplication";
            warnedNoWidgets = true;
        }
        show = false;
    }

    if (show) {
        if (style() != ApplicationMenuStyle::SingleButton) {
            const auto actions = menu->actions();
            for (QAction* action : actions) {
                TitleBarMenuItem* item = factory->createItem(q);
                item->setAction(action);
                // GTK applications start with an unlabelled item.
                if (item->text().isEmpty()) {
                    item->setEnabled(false);
                    item->setVisible(false);
                }
                items.append(item);
                q->addButton(item);
            }
        }
        overflowButton = factory->createOverflowButton(q);
        q->addButton(overflowButton);
        if (decoration->settings()->isApplicationMenuSearchEnabled()) {
            searchButton = factory->createSearchButton(q);
            q->addButton(searchButton);
        }

        const auto buttons = q->buttons();
        for (int i = 0; i < buttons.size(); ++i) {
            DecorationButton* button = buttons.at(i);
            button->setCheckable(true);
            QObject::connect(button, &DecorationButton::clicked, q, [this, i](Qt::MouseButton mouseButton) {
                if (mouseButton == Qt::LeftButton) {
                    q->openItem(i);
                }
            });
        }
    }

    updateHandler();
    if (!availableRect.isNull()) {
        q->updateItemsGeometry(availableRect, iconButtonSize);
    } else {
        setVisibleWidth(0);
    }
    Q_EMIT q->itemsChanged();
}

void TitleBarMenuPrivate::updateHandler()
{
    if (!decoration) {
        return;
    }
    const bool wanted = anchor && anchor->isVisible();
    if (wanted == handlerSet) {
        return;
    }
    handlerSet = wanted;
    if (wanted) {
        decoration->setApplicationMenuHandler([menu = QPointer<TitleBarMenu>(q)](int actionId) {
            if (menu) {
                menu->d->handleShowRequest(actionId);
            }
        });
    } else {
        decoration->setApplicationMenuHandler({});
    }
}

void TitleBarMenuPrivate::handleShowRequest(int actionId)
{
    if (q->buttons().isEmpty()) {
        return;
    }
    if (style() == ApplicationMenuStyle::SingleButton) {
        q->openOverflow();
        return;
    }

    int index = -1;
    QMenu* root = model->menu();
    if (QAction* action = model->actionForId(actionId); action && root) {
        // Walk up to the top-level action that holds it.
        QAction* top = action;
        QSet<QMenu*> visited;
        while (auto parentMenu = qobject_cast<QMenu*>(top->parent())) {
            if (parentMenu == root || visited.contains(parentMenu) || !parentMenu->menuAction()) {
                break;
            }
            visited.insert(parentMenu);
            top = parentMenu->menuAction();
        }
        index = root->actions().indexOf(top);
    }
    if (index < 0 || index >= items.size() || !items.at(index) || !items.at(index)->isVisible() || !items.at(index)->isEnabled()) {
        index = nextVisibleItemIndex(-1, true);
    }
    if (index < 0) {
        return;
    }
    q->openItem(index);
}

void TitleBarMenuPrivate::updateShowing()
{
    if (style() != ApplicationMenuStyle::ShownOnHover) {
        animation->stop();
        setOpacity(1);
        return;
    }
    const bool showing = hovered || q->isPopupOpen();
    if (!animationsEnabled || animation->duration() <= 0) {
        animation->stop();
        setOpacity(showing ? 1 : 0);
        return;
    }
    const auto direction = showing ? QAbstractAnimation::Forward : QAbstractAnimation::Backward;
    if (animation->state() == QAbstractAnimation::Running) {
        if (animation->direction() != direction) {
            animation->setDirection(direction);
        }
        return;
    }
    if (qFuzzyCompare(opacity, showing ? 1.0 : 0.0)) {
        return;
    }
    animation->setDirection(direction);
    animation->start();
}

void TitleBarMenuPrivate::setOpacity(qreal value)
{
    if (qFuzzyCompare(opacity, value)) {
        return;
    }
    opacity = value;
    if (decoration) {
        decoration->update(q->geometry());
    }
    Q_EMIT q->opacityChanged(opacity);
}

void TitleBarMenuPrivate::setVisibleWidth(qreal width)
{
    if (qFuzzyCompare(visibleWidth + 1, width + 1)) {
        return;
    }
    visibleWidth = width;
    Q_EMIT q->visibleItemsWidthChanged();
}

void TitleBarMenuPrivate::showPopup(QMenu* menu, int index)
{
    model->stopCaching();
    DecorationButton* button = q->buttons().value(index);
    if (!menu || !button || !decoration) {
        return;
    }

    QMenu* oldMenu = openPopup;
    DecorationButton* oldButton = openIndex >= 0 ? q->buttons().value(openIndex) : nullptr;

    openIndex = index;
    openPopup = menu;
    button->setChecked(true);

    QObject::disconnect(menu, nullptr, q, nullptr);
    if (auto popup = qobject_cast<TitleBarMenuPopup*>(menu)) {
        QObject::connect(popup, &TitleBarMenuPopup::previousItemRequested, q, &TitleBarMenu::openPreviousItem);
        QObject::connect(popup, &TitleBarMenuPopup::nextItemRequested, q, &TitleBarMenu::openNextItem);
    }
    QObject::connect(menu, &QMenu::aboutToHide, q, [this, menu = QPointer<QMenu>(menu)] {
        popupAboutToHide(menu);
    });
    menu->installEventFilter(q);

    sonicDebug() << "Opening the window menu popup of item" << index;
    Positioner positioner;
    positioner.setAnchorRect(button->geometry());
    decoration->popup(positioner, menu);

    if (menu == searchPopup && searchLineEdit) {
        searchLineEdit->activateWindow();
        searchLineEdit->setFocus();
    }

    if (oldMenu && oldMenu != menu) {
        oldMenu->close(); // close(), so an overflow popup deletes itself
    }
    if (oldButton && oldButton != button) {
        oldButton->setChecked(false);
    }

    updateShowing();
    decoration->update(q->geometry());

    // Load what the user is likely to open next, after the popup has appeared.
    menuToCache = menu;
    menuToCacheIndex = index;
    cacheTimer.start();
}

void TitleBarMenuPrivate::popupAboutToHide(QMenu* menu)
{
    if (menu && menu == searchPopup) {
        searchPending = false;
        // QMenu hides before it activates the chosen result, so the results stay until
        // the cleared text's update removes them.
        if (searchLineEdit) {
            searchLineEdit->clear();
        }
    }
    if (!menu || menu != openPopup) {
        return;
    }
    if (DecorationButton* button = q->buttons().value(openIndex)) {
        button->setChecked(false);
    }
    openIndex = -1;
    openPopup.clear();
    hoveredButton.clear();
    updateShowing();
    if (decoration) {
        decoration->update(q->geometry());
    }
}

int TitleBarMenuPrivate::nextVisibleItemIndex(int from, bool forward) const
{
    const auto buttons = q->buttons();
    if (buttons.isEmpty()) {
        return -1;
    }
    const int step = forward ? 1 : -1;
    int index = from;
    for (int i = 0; i < buttons.size(); ++i) {
        index += step;
        if (index < 0) {
            index = buttons.size() - 1;
        } else if (index >= buttons.size()) {
            index = 0;
        }
        const DecorationButton* button = buttons.at(index);
        if (index != from && button->isVisible() && button->isEnabled()) {
            return index;
        }
    }
    return from;
}

void TitleBarMenuPrivate::ensureSearchPopup()
{
    if (searchPopup) {
        return;
    }
    searchPopup = new TitleBarMenuPopup();
    searchLineEdit = new QLineEdit();
    searchLineEdit->setMinimumWidth(searchLineEdit->fontMetrics().averageCharWidth() * 30);
    searchLineEdit->setPlaceholderText(i18nc("@info:placeholder", "Search…"));
    searchLineEdit->setFocusPolicy(Qt::StrongFocus);
    auto searchAction = new QWidgetAction(searchPopup);
    searchAction->setDefaultWidget(searchLineEdit); // the action owns the line edit
    searchPopup->addAction(searchAction);
    searchPopup->addSeparator();

    searchLineEdit->installEventFilter(q);
    QObject::connect(searchLineEdit, &QLineEdit::textChanged, &searchTimer, qOverload<>(&QTimer::start));
}

void TitleBarMenuPrivate::repositionSearchPopup()
{
    if (!searchPopup || !searchPopup->isVisible() || !searchButton || !decoration) {
        return;
    }
    searchPopup->adjustSize();
    Positioner positioner;
    positioner.setAnchorRect(searchButton->geometry());
    decoration->popup(positioner, searchPopup);
}

void TitleBarMenuPrivate::clearSearchResults()
{
    if (!searchPopup) {
        return;
    }
    for (const auto& action : std::as_const(searchResultActions)) {
        if (action) {
            searchPopup->removeAction(action); // doesn't delete it
            delete action.data();
        }
    }
    searchResultActions.clear();
}

TitleBarMenuPrivate::ActionInfo TitleBarMenuPrivate::actionPath(QAction* action)
{
    QStringList path{action->text().remove(QLatin1Char('&'))};
    bool enabled = action->isEnabled();
    QSet<QMenu*> visited;
    auto menu = qobject_cast<QMenu*>(action->parent());
    while (menu && !visited.contains(menu)) {
        visited.insert(menu);
        QAction* parentAction = menu->menuAction();
        if (!parentAction) {
            break;
        }
        if (!parentAction->isEnabled()) {
            enabled = false;
        }
        const QString text = parentAction->text().remove(QLatin1Char('&'));
        if (!text.isEmpty()) {
            path.prepend(text);
        }
        menu = qobject_cast<QMenu*>(parentAction->parent());
    }
    return {path.join(QStringLiteral(" » ")), enabled};
}

void TitleBarMenuPrivate::collectSearchResults(QMenu* menu, const QString& text, QList<QAction*>& results, QSet<QMenu*>& visited) const
{
    if (visited.contains(menu)) {
        return;
    }
    visited.insert(menu);
    const auto actions = menu->actions();
    for (QAction* action : actions) {
        if (action->isSeparator()) {
            continue;
        }
        if (action->menu()) {
            collectSearchResults(action->menu(), text, results, visited);
            continue;
        }
        const ActionInfo info = actionPath(action);
        if (info.enabled && info.path.contains(text, Qt::CaseInsensitive)) {
            results.append(action);
        }
    }
}

TitleBarMenu::TitleBarMenu(Decoration* decoration, std::unique_ptr<TitleBarMenuFactory> factory)
    : DecorationButtonGroup(decoration)
    , d(std::make_unique<TitleBarMenuPrivate>(this, std::move(factory)))
{
    setSpacing(0);

    d->addressTimer.setSingleShot(true);
    d->addressTimer.setInterval(100);
    connect(&d->addressTimer, &QTimer::timeout, this, [this] {
        if (d->addressPending) {
            d->addressPending = false;
            d->onAddressChanged();
        }
    });

    d->cacheTimer.setSingleShot(true);
    d->cacheTimer.setInterval(200);
    connect(&d->cacheTimer, &QTimer::timeout, this, [this] {
        if (!d->menuToCache || d->openPopup != d->menuToCache) {
            return;
        }
        if (d->menuToCache == d->searchPopup) {
            d->model->cacheAll();
        } else {
            d->model->cacheSubmenus(d->menuToCache);
        }
    });

    d->searchTimer.setSingleShot(true);
    d->searchTimer.setInterval(150);
    connect(&d->searchTimer, &QTimer::timeout, this, [this] {
        if (d->searchLineEdit) {
            updateSearchResults(d->searchLineEdit->text());
        }
    });

    d->animation->setStartValue(0.0);
    d->animation->setEndValue(1.0);
    d->animation->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->animation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
        d->setOpacity(value.toReal());
    });

    connect(d->model, &TitleBarMenuModel::itemsChanged, this, [this] {
        d->updateItems();
    });
    connect(d->model, &TitleBarMenuModel::submenuReady, this, [this](QMenu* menu) {
        if (d->waitingIndex < 0) {
            return;
        }
        const int index = d->waitingIndex;
        TitleBarMenuItem* item = d->items.value(index);
        if (item && item->action() && item->action()->menu() == menu) {
            d->waitingIndex = -1;
            openItem(index);
        }
    });
    connect(d->model, &TitleBarMenuModel::searchReady, this, [this] {
        if (d->searchPending && d->openPopup && d->openPopup == d->searchPopup) {
            updateSearchResults(d->searchText);
        }
    });

    const auto settings = decoration->settings().get();
    connect(settings, &DecorationSettings::applicationMenuStyleChanged, this, [this] {
        d->updateItems();
        d->updateShowing();
    });
    connect(settings, &DecorationSettings::applicationMenuSearchEnabledChanged, this, [this] {
        d->updateItems();
    });

    const auto window = decoration->window();
    connect(window, &DecoratedWindow::hasApplicationMenuChanged, this, [this] {
        d->onAddressChanged();
    });
    connect(window, &DecoratedWindow::applicationMenuChanged, this, [this] {
        d->onAddressChanged();
    });

    d->updateShowing();
    if (window->hasApplicationMenu()) {
        d->applyAddress();
        d->addressTimer.start();
    }
}

TitleBarMenu::~TitleBarMenu()
{
    if (d->handlerSet && d->decoration) {
        d->decoration->setApplicationMenuHandler({});
    }
    if (d->anchor && d->decoration) {
        d->anchor->setEnabled(true);
    }
    delete d->searchPopup.data();
}

void TitleBarMenu::setAnchor(DecorationButton* anchor)
{
    if (d->anchor != anchor) {
        if (d->anchor) {
            d->anchor->setEnabled(true);
        }
        disconnect(d->anchorVisibility);
        d->anchor = anchor;
        if (anchor) {
            d->anchorVisibility = connect(anchor, &DecorationButton::visibilityChanged, this, [this] {
                d->updateItems();
            });
        }
    }
    if (anchor) {
        // Buttons of a recreated group start enabled; the anchor never takes clicks.
        anchor->setEnabled(false);
    }
    d->updateItems();
}

DecorationButton* TitleBarMenu::anchor() const
{
    return d->anchor;
}

void TitleBarMenu::updateItemsGeometry(const QRectF& availableRect, const QSizeF& iconButtonSize)
{
    d->availableRect = availableRect;
    d->iconButtonSize = iconButtonSize;

    const auto allButtons = buttons();
    if (availableRect.isNull()) {
        for (DecorationButton* button : allButtons) {
            button->setVisible(false);
        }
        d->setVisibleWidth(0);
        return;
    }

    const qreal height = availableRect.height();
    const QRectF iconRect(QPointF(0, 0), iconButtonSize);
    if (d->overflowButton) {
        d->overflowButton->setGeometry(iconRect);
    }
    if (d->searchButton) {
        d->searchButton->setGeometry(iconRect);
        d->searchButton->setVisible(true);
    }
    for (const auto& item : std::as_const(d->items)) {
        if (item) {
            item->setGeometry(QRectF(0, 0, item->preferredWidth(), height));
        }
    }

    if (d->style() == ApplicationMenuStyle::SingleButton) {
        if (d->overflowButton) {
            d->overflowButton->setVisible(true);
        }
    } else {
        const qreal searchWidth = d->searchButton ? iconButtonSize.width() : 0;
        qreal itemsWidth = 0;
        for (const auto& item : std::as_const(d->items)) {
            if (item && item->isEnabled()) {
                itemsWidth += item->geometry().width();
            }
        }
        const bool overflowing = itemsWidth + searchWidth > availableRect.width();
        const qreal room = availableRect.width() - searchWidth - (overflowing ? iconButtonSize.width() : 0);
        qreal x = 0;
        bool full = false;
        for (const auto& item : std::as_const(d->items)) {
            if (!item || !item->isEnabled()) {
                continue;
            }
            const qreal width = item->geometry().width();
            full = full || x + width > room;
            item->setVisible(!full);
            if (!full) {
                x += width;
            }
        }
        if (d->overflowButton) {
            d->overflowButton->setVisible(overflowing);
        }
    }

    qreal width = 0;
    int visibleCount = 0;
    for (DecorationButton* button : buttons()) {
        if (button->isVisible()) {
            width += button->geometry().width();
            ++visibleCount;
        }
    }
    if (visibleCount > 1) {
        width += spacing() * (visibleCount - 1);
    }
    d->setVisibleWidth(width);
}

qreal TitleBarMenu::visibleItemsWidth() const
{
    return d->visibleWidth;
}

qreal TitleBarMenu::opacity() const
{
    return d->opacity;
}

void TitleBarMenu::setHovered(bool hovered)
{
    if (d->hovered == hovered) {
        return;
    }
    d->hovered = hovered;
    d->updateShowing();
}

bool TitleBarMenu::isHovered() const
{
    return d->hovered;
}

void TitleBarMenu::setAnimationsEnabled(bool enabled)
{
    d->animationsEnabled = enabled;
}

void TitleBarMenu::setAnimationDuration(int milliseconds)
{
    d->animation->setDuration(milliseconds);
}

bool TitleBarMenu::isPopupOpen() const
{
    return d->openIndex >= 0;
}

int TitleBarMenu::openItemIndex() const
{
    return d->openIndex;
}

void TitleBarMenu::openItem(int index)
{
    DecorationButton* button = buttons().value(index);
    if (!button) {
        return;
    }
    if (button == d->searchButton) {
        openSearch();
        return;
    }
    if (button == d->overflowButton) {
        openOverflow();
        return;
    }

    TitleBarMenuItem* item = d->items.value(index);
    if (!item || !item->action() || !item->action()->menu()) {
        return;
    }
    QMenu* submenu = item->action()->menu();
    if (submenu->actions().isEmpty()) {
        // Imported submenus load on demand; open it once it's there.
        if (d->waitingIndex == index) {
            return;
        }
        d->waitingIndex = index;
        d->model->loadSubmenu(submenu);
        return;
    }
    d->waitingIndex = -1;
    d->showPopup(submenu, index);
}

void TitleBarMenu::openOverflow()
{
    if (!d->overflowButton || !d->overflowButton->isVisible() || !d->model->menu()) {
        return;
    }
    const int index = buttons().indexOf(d->overflowButton.data());
    auto menu = new TitleBarMenuPopup();
    menu->setAttribute(Qt::WA_DeleteOnClose);

    const auto actions = d->model->menu()->actions();
    int first = 0;
    if (d->style() != ApplicationMenuStyle::SingleButton) {
        first = actions.size();
        for (int i = 0; i < d->items.size(); ++i) {
            const auto& item = d->items.at(i);
            if (item && item->isEnabled() && !item->isVisible()) {
                first = i;
                break;
            }
        }
    }
    for (int i = first; i < actions.size(); ++i) {
        menu->addAction(actions.at(i)); // the importer's menus keep owning the actions
    }
    d->showPopup(menu, index);
}

void TitleBarMenu::openSearch()
{
    if (!d->searchButton || !d->widgetsAvailable()) {
        return;
    }
    if (d->openPopup && d->openPopup == d->searchPopup) {
        d->searchPopup->hide();
        return;
    }
    d->ensureSearchPopup();
    d->showPopup(d->searchPopup, buttons().indexOf(d->searchButton.data()));
}

void TitleBarMenu::openPreviousItem()
{
    const int index = d->nextVisibleItemIndex(d->openIndex, false);
    if (index >= 0 && index != d->openIndex) {
        openItem(index);
    }
}

void TitleBarMenu::openNextItem()
{
    const int index = d->nextVisibleItemIndex(d->openIndex, true);
    if (index >= 0 && index != d->openIndex) {
        openItem(index);
    }
}

void TitleBarMenu::updateHoveredItem(const QPointF& pos)
{
    if (!isPopupOpen()) {
        return;
    }
    const auto allButtons = buttons();
    DecorationButton* hovered = nullptr;
    for (DecorationButton* button : allButtons) {
        if (button->isVisible() && button->isEnabled() && button->contains(pos)) {
            hovered = button;
            break;
        }
    }
    if (hovered == d->hoveredButton) {
        return;
    }
    d->hoveredButton = hovered;
    if (hovered) {
        const int index = allButtons.indexOf(hovered);
        if (index != d->openIndex) {
            openItem(index);
        }
    }
}

void TitleBarMenu::releaseAllItems()
{
    // Disabling a button clears its pressed state.
    for (DecorationButton* button : buttons()) {
        if (button->isEnabled()) {
            button->setEnabled(false);
            button->setEnabled(true);
        }
    }
}

QList<QAction*> TitleBarMenu::collectSearchResults(const QString& text) const
{
    QList<QAction*> results;
    if (text.length() < 3 || !d->model->menu()) {
        return results;
    }
    QSet<QMenu*> visited;
    d->collectSearchResults(d->model->menu(), text, results, visited);
    return results;
}

void TitleBarMenu::updateSearchResults(const QString& text)
{
    d->ensureSearchPopup();
    d->searchText = text;
    d->clearSearchResults();
    d->searchPending = false;
    if (text.length() < 3) {
        d->repositionSearchPopup();
        return;
    }
    if (d->model->isCaching()) {
        d->searchPending = true;
        return;
    }

    const auto results = collectSearchResults(text);
    for (QAction* action : results) {
        auto result = new QAction(action->icon(), TitleBarMenuPrivate::actionPath(action).path, d->searchPopup);
        result->setCheckable(action->isCheckable());
        result->setChecked(action->isChecked());
        connect(result, &QAction::triggered, this, [this, action = QPointer<QAction>(action)] {
            if (action) {
                action->trigger();
            }
            if (d->searchPopup) {
                d->searchPopup->hide();
            }
        });
        d->searchPopup->addAction(result);
        d->searchResultActions.append(result);
    }
    d->repositionSearchPopup();
}

QMenu* TitleBarMenu::searchPopup() const
{
    return d->searchPopup;
}

TitleBarMenuModel* TitleBarMenu::model() const
{
    return d->model;
}

QList<TitleBarMenuItem*> TitleBarMenu::items() const
{
    QList<TitleBarMenuItem*> result;
    for (const auto& item : std::as_const(d->items)) {
        if (item) {
            result.append(item);
        }
    }
    return result;
}

DecorationButton* TitleBarMenu::overflowButton() const
{
    return d->overflowButton;
}

DecorationButton* TitleBarMenu::searchButton() const
{
    return d->searchButton;
}

void TitleBarMenu::paint(QPainter* painter, const QRectF& repaintArea)
{
    if (d->opacity <= 0) {
        return;
    }
    painter->save();
    painter->setOpacity(painter->opacity() * d->opacity);
    DecorationButtonGroup::paint(painter, repaintArea);
    painter->restore();
}

bool TitleBarMenu::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == d->searchLineEdit && event->type() == QEvent::KeyPress) {
        auto keyEvent = static_cast<QKeyEvent*>(event);
        // The field keeps the focus: Up/Down move through the results, Return picks one.
        QList<QAction*> results;
        for (const auto& action : std::as_const(d->searchResultActions)) {
            if (action) {
                results.append(action);
            }
        }
        const int active = results.indexOf(d->searchPopup->activeAction());
        if ((keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) && !results.isEmpty()) {
            const int next = keyEvent->key() == Qt::Key_Down ? qMin(active + 1, int(results.size()) - 1) : qMax(active - 1, 0);
            d->searchPopup->setActiveAction(results.at(next));
            return true;
        }
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && !results.isEmpty()) {
            results.at(qMax(active, 0))->trigger();
            return true;
        }
        if (keyEvent->modifiers() == Qt::NoModifier) {
            const bool atStart = keyEvent->key() == Qt::Key_Left && d->searchLineEdit->cursorPosition() == 0;
            const bool atEnd = keyEvent->key() == Qt::Key_Right && d->searchLineEdit->cursorPosition() == d->searchLineEdit->text().length();
            if ((atStart || atEnd) && d->searchPopup) {
                QCoreApplication::sendEvent(d->searchPopup, keyEvent);
                return true;
            }
        }
        return false;
    }

    // Popups grab the pointer, so the decoration sees no motion while one is open.
    if (event->type() == QEvent::MouseMove && qobject_cast<QMenu*>(watched) && d->decoration) {
        if (const auto position = d->decoration->window()->position()) {
            const auto mouseEvent = static_cast<QMouseEvent*>(event);
            updateHoveredItem(mouseEvent->globalPosition() - *position);
        }
    }
    return DecorationButtonGroup::eventFilter(watched, event);
}

}

#include "moc_titlebarmenu.cpp"
