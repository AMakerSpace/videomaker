/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#include "amakertheme.h"

#include <KConfigGroup>
#include <KSharedConfig>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QStyleHints>

namespace {

KConfigGroup settings()
{
    return KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("VideoMaker"));
}

void applyStyleSheet()
{
    QFile qss(AMakerTheme::isDark() ? QStringLiteral(":/videomaker/theme/amaker-dark.qss") : QStringLiteral(":/videomaker/theme/amaker-light.qss"));
    if (qss.open(QIODevice::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));
    }
}

void storeScheme()
{
    KConfigGroup ui(KSharedConfig::openConfig(), QStringLiteral("UiSettings"));
    ui.writeEntry("ColorSchemePath", AMakerTheme::schemeFile());
    ui.sync();
}

} // namespace

void AMakerTheme::setupLanguage()
{
    // German by default, English on request
    const QString lang = settings().readEntry("Sprache", QStringLiteral("de"));
    qputenv("LANGUAGE", lang == QLatin1String("en") ? "en_GB" : "de");
}

void AMakerTheme::setup()
{
    const int id = QFontDatabase::addApplicationFont(QStringLiteral(":/videomaker/fonts/StackSansNotch-VariableFont_wght.ttf"));
    if (id >= 0) {
        QFont font = QGuiApplication::font();
        font.setFamily(QFontDatabase::applicationFontFamilies(id).value(0, QStringLiteral("Stack Sans Notch")));
        font.setWeight(QFont::Medium);
        QGuiApplication::setFont(font);
    }
    storeScheme();
    applyStyleSheet();
}

bool AMakerTheme::isDark()
{
    const QString mode = settings().readEntry("Thema", QStringLiteral("auto"));
    if (mode == QLatin1String("dunkel")) {
        return true;
    }
    if (mode == QLatin1String("hell")) {
        return false;
    }
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString AMakerTheme::schemeFile()
{
    return isDark() ? QStringLiteral("AMakerDark.colors") : QStringLiteral("AMakerLight.colors");
}

bool AMakerTheme::motionEnabled()
{
    if (settings().readEntry("Bewegung", QStringLiteral("an")) == QLatin1String("aus")) {
        return false;
    }
    // the launcher sets VIDEOMAKER_NO_MOTION when GNOME animations are switched off
    return !qEnvironmentVariableIsSet("VIDEOMAKER_NO_MOTION");
}

void AMakerTheme::setMode(const QString &mode)
{
    KConfigGroup cg = settings();
    cg.writeEntry("Thema", mode);
    cg.sync();
    storeScheme();
    applyStyleSheet();
}

void AMakerTheme::watchSystem(const std::function<void(bool dark)> &onChange)
{
    QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, qApp, [onChange]() {
        if (settings().readEntry("Thema", QStringLiteral("auto")) != QLatin1String("auto")) {
            return;
        }
        storeScheme();
        applyStyleSheet();
        onChange(isDark());
    });
}
