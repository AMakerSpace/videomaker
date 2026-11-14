/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once

#include <QString>
#include <functional>

/**
 * @brief VideoMaker: AMaker design language (fonts, light/dark/auto, stylesheet, language).
 * Settings live in the [VideoMaker] group of videomakerrc: Sprache=de|en, Thema=auto|hell|dunkel, Bewegung=an|aus.
 */
namespace AMakerTheme {

/** @brief Call before the first i18n(): selects German (default) or English. */
void setupLanguage();
/** @brief Call after the application object exists: font, color scheme selection, stylesheet. */
void setup();
/** @brief true when the dark variant is active (theme setting or, for "auto", the system scheme). */
bool isDark();
/** @brief File name of the KDE color scheme for the current mode (AMakerLight.colors / AMakerDark.colors). */
QString schemeFile();
/** @brief false when the user or the system asked for reduced motion. */
bool motionEnabled();
/** @brief Store Thema=auto|hell|dunkel and restyle the application (palette switch is up to the caller). */
void setMode(const QString &mode);
/** @brief Follow system light/dark changes while Thema=auto; callback gets the new mode. */
void watchSystem(const std::function<void(bool dark)> &onChange);

} // namespace AMakerTheme
