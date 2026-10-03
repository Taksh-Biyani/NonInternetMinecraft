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

#include "Accessibility.h"

#include <QApplication>
#include <algorithm>
#include <cmath>
#include <cstdlib>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace Accessibility {

Contrast contrastFromString(const QString& value)
{
    if (value == "Off")
        return Contrast::Off;
    if (value == "Dark")
        return Contrast::Dark;
    if (value == "Light")
        return Contrast::Light;
    return Contrast::Auto;
}

QString contrastToString(Contrast contrast)
{
    switch (contrast) {
        case Contrast::Off: return "Off";
        case Contrast::Dark: return "Dark";
        case Contrast::Light: return "Light";
        case Contrast::Auto: break;
    }
    return "Auto";
}

QString effectiveTheme(Contrast setting, const QString& chosenTheme, bool windowsHighContrast, bool windowsHighContrastIsLight)
{
    switch (setting) {
        case Contrast::Off: return chosenTheme;
        case Contrast::Dark: return HighContrastDarkTheme;
        case Contrast::Light: return HighContrastLightTheme;
        case Contrast::Auto: break;
    }
    if (!windowsHighContrast)
        return chosenTheme;
    return windowsHighContrastIsLight ? HighContrastLightTheme : HighContrastDarkTheme;
}

QList<int> textScales()
{
    return { 100, 125, 150, 175, 200 };
}

int clampTextScale(int percent)
{
    if (percent <= 0)
        return 100;
    int best = 100;
    for (int scale : textScales()) {
        if (std::abs(scale - percent) < std::abs(best - percent))
            best = scale;
    }
    return best;
}

QFont scaledFont(const QFont& base, int percent)
{
    QFont font = base;
    const double factor = clampTextScale(percent) / 100.0;
    if (base.pointSizeF() > 0)
        font.setPointSizeF(base.pointSizeF() * factor);
    else if (base.pixelSize() > 0)
        font.setPixelSize(qRound(base.pixelSize() * factor));
    return font;
}

static double linear(double channel)
{
    return channel <= 0.04045 ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
}

static double luminance(const QColor& color)
{
    return 0.2126 * linear(color.redF()) + 0.7152 * linear(color.greenF()) + 0.0722 * linear(color.blueF());
}

double contrastRatio(const QColor& a, const QColor& b)
{
    const double la = luminance(a);
    const double lb = luminance(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

bool windowsHighContrastOn(bool* isLight)
{
    if (isLight)
        *isLight = false;
#ifdef Q_OS_WIN
    HIGHCONTRASTW info{};
    info.cbSize = sizeof(info);
    if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(info), &info, 0) || !(info.dwFlags & HCF_HIGHCONTRASTON))
        return false;
    if (isLight) {
        const COLORREF window = GetSysColor(COLOR_WINDOW);
        *isLight = luminance(QColor(GetRValue(window), GetGValue(window), GetBValue(window))) > 0.5;
    }
    return true;
#else
    return false;
#endif
}

void applyTextScale(int percent)
{
    static const QFont base = QApplication::font();
    QApplication::setFont(scaledFont(base, percent));
}

}  // namespace Accessibility
