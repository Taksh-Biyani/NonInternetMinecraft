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

#pragma once

#include <QColor>
#include <QFont>
#include <QList>
#include <QString>

// Launcher accessibility options (Settings → Accessibility): text size and high-contrast colours.
namespace Accessibility {

enum class Contrast { Auto, Off, Dark, Light };

inline const QString HighContrastDarkTheme = QStringLiteral("pinecone-high-contrast-dark");
inline const QString HighContrastLightTheme = QStringLiteral("pinecone-high-contrast-light");

Contrast contrastFromString(const QString& value);  // unknown values mean Auto
QString contrastToString(Contrast contrast);

// The theme to apply: a high-contrast one when the setting (or, for Auto, Windows) asks for it, else `chosenTheme`.
QString effectiveTheme(Contrast setting, const QString& chosenTheme, bool windowsHighContrast, bool windowsHighContrastIsLight);

QList<int> textScales();          // 100, 125, 150, 175, 200 (percent)
int clampTextScale(int percent);  // the nearest allowed value; 100 for nonsense
QFont scaledFont(const QFont& base, int percent);

// WCAG 2 contrast ratio, from 1.0 (none) to 21.0 (black on white).
double contrastRatio(const QColor& a, const QColor& b);

// Whether Windows' high-contrast mode is on, and whether its background is light. Always false on other systems.
bool windowsHighContrastOn(bool* isLight = nullptr);

// Scales the application font. Remembers the unscaled font the first time, so repeated calls don't compound.
void applyTextScale(int percent);

}  // namespace Accessibility
