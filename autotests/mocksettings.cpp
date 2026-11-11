/*
 * SPDX-FileCopyrightText: 2014 Martin Gräßlin <mgraesslin@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */
#include "mocksettings.h"
#include "../src/decorationsettings.h"

MockSettings::MockSettings(KDecoration3::DecorationSettings* parent)
    : DecorationSettingsPrivateV3(parent)
{
}

KDecoration3::BorderSize MockSettings::borderSize() const
{
    return KDecoration3::BorderSize::Normal;
}

QList<KDecoration3::DecorationButtonType> MockSettings::decorationButtonsLeft() const
{
    return QList<KDecoration3::DecorationButtonType>();
}

QList<KDecoration3::DecorationButtonType> MockSettings::decorationButtonsRight() const
{
    return QList<KDecoration3::DecorationButtonType>();
}

bool MockSettings::isAlphaChannelSupported() const
{
    return true;
}

bool MockSettings::isCloseOnDoubleClickOnMenu() const
{
    return m_closeDoubleClickOnMenu;
}

bool MockSettings::isOnAllDesktopsAvailable() const
{
    return m_onAllDesktopsAvailable;
}

bool MockSettings::isAlwaysShowExcludeFromCapture() const
{
    return m_alwaysShowExcludeFromCapture;
}

void MockSettings::setOnAllDesktopsAvailabe(bool set)
{
    if (m_onAllDesktopsAvailable == set) {
        return;
    }
    m_onAllDesktopsAvailable = set;
    Q_EMIT decorationSettings()->onAllDesktopsAvailableChanged(m_onAllDesktopsAvailable);
}

void MockSettings::setCloseOnDoubleClickOnMenu(bool set)
{
    if (m_closeDoubleClickOnMenu == set) {
        return;
    }
    m_closeDoubleClickOnMenu = set;
    Q_EMIT decorationSettings()->closeOnDoubleClickOnMenuChanged(m_closeDoubleClickOnMenu);
}

void MockSettings::setAlwaysShowExcludeFromCapture(bool set)
{
    if (m_alwaysShowExcludeFromCapture == set) {
        return;
    }
    m_alwaysShowExcludeFromCapture = set;
    Q_EMIT decorationSettings()->alwaysShowExcludeFromCaptureChanged(m_alwaysShowExcludeFromCapture);
}

KDecoration3::ApplicationMenuStyle MockSettings::applicationMenuStyle() const
{
    return m_applicationMenuStyle;
}

bool MockSettings::isApplicationMenuSearchEnabled() const
{
    return m_applicationMenuSearchEnabled;
}

void MockSettings::setApplicationMenuStyle(KDecoration3::ApplicationMenuStyle style)
{
    if (m_applicationMenuStyle == style) {
        return;
    }
    m_applicationMenuStyle = style;
    Q_EMIT decorationSettings()->applicationMenuStyleChanged(m_applicationMenuStyle);
}

void MockSettings::setApplicationMenuSearchEnabled(bool set)
{
    if (m_applicationMenuSearchEnabled == set) {
        return;
    }
    m_applicationMenuSearchEnabled = set;
    Q_EMIT decorationSettings()->applicationMenuSearchEnabledChanged(m_applicationMenuSearchEnabled);
}
