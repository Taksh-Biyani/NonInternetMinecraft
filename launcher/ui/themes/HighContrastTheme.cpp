// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PineconeMC Offline - Minecraft Launcher
 *  Copyright (C) 2026 PineconeMC Offline Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "HighContrastTheme.h"

#include <QObject>

#include "Accessibility.h"

namespace {
struct Colors {
    QColor background, text, accent, accentText, link, disabled, focus;
};

Colors colors(bool dark)
{
    if (dark)
        return { QColor(0, 0, 0),     QColor(255, 255, 255), QColor(26, 235, 255), QColor(0, 0, 0),
                 QColor(255, 255, 0), QColor(170, 170, 170), QColor(255, 255, 0) };
    return { QColor(255, 255, 255), QColor(0, 0, 0),    QColor(55, 0, 110), QColor(255, 255, 255),
             QColor(0, 0, 159),     QColor(85, 85, 85), QColor(0, 0, 255) };
}
}  // namespace

QString HighContrastTheme::id()
{
    return m_dark ? Accessibility::HighContrastDarkTheme : Accessibility::HighContrastLightTheme;
}

QString HighContrastTheme::name()
{
    return m_dark ? QObject::tr("High contrast (dark)") : QObject::tr("High contrast (light)");
}

QString HighContrastTheme::tooltip()
{
    return QObject::tr("Strong colours and a thick outline around whatever the keyboard is on.");
}

QColor HighContrastTheme::fadeColor()
{
    return colors(m_dark).background;
}

QPalette HighContrastTheme::colorScheme()
{
    const Colors c = colors(m_dark);
    QPalette palette;
    for (auto group : { QPalette::Active, QPalette::Inactive, QPalette::Disabled }) {
        const QColor text = group == QPalette::Disabled ? c.disabled : c.text;
        palette.setColor(group, QPalette::Window, c.background);
        palette.setColor(group, QPalette::WindowText, text);
        palette.setColor(group, QPalette::Base, c.background);
        palette.setColor(group, QPalette::AlternateBase, c.background);
        palette.setColor(group, QPalette::ToolTipBase, c.background);
        palette.setColor(group, QPalette::ToolTipText, c.text);
        palette.setColor(group, QPalette::PlaceholderText, c.disabled);
        palette.setColor(group, QPalette::Text, text);
        palette.setColor(group, QPalette::Button, c.background);
        palette.setColor(group, QPalette::ButtonText, text);
        palette.setColor(group, QPalette::BrightText, c.link);
        palette.setColor(group, QPalette::Light, c.text);
        palette.setColor(group, QPalette::Midlight, c.text);
        palette.setColor(group, QPalette::Mid, c.disabled);
        palette.setColor(group, QPalette::Dark, c.text);
        palette.setColor(group, QPalette::Shadow, c.text);
        palette.setColor(group, QPalette::Highlight, c.accent);
        palette.setColor(group, QPalette::HighlightedText, c.accentText);
        palette.setColor(group, QPalette::Link, c.link);
        palette.setColor(group, QPalette::LinkVisited, c.link);
    }
    return palette;
}

QString HighContrastTheme::appStyleSheet()
{
    const Colors c = colors(m_dark);
    // Clear borders on buttons and text fields, and a thick outline on whatever has keyboard focus. Combo and spin boxes
    // are left to the style: a style-sheet border would hide their arrows.
    return QString(
               "QToolTip { color: %1; background-color: %2; border: 2px solid %1; }"
               "QPushButton { color: %1; background-color: %2; border: 2px solid %1; padding: 4px 12px; }"
               "QPushButton:hover { border-color: %3; }"
               "QPushButton:pressed, QPushButton:checked { color: %4; background-color: %3; }"
               "QPushButton:disabled { color: %5; border: 2px dashed %5; }"
               "QPushButton:focus { border: 3px solid %6; }"
               "QLineEdit, QPlainTextEdit, QTextEdit { border: 2px solid %1; }"
               "QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QAbstractItemView:focus { border: 3px solid %6; }")
        .arg(c.text.name(), c.background.name(), c.accent.name(), c.accentText.name(), c.disabled.name(), c.focus.name());
}
