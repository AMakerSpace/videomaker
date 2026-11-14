/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once

#include <QIcon>
#include <QObject>
#include <QPointer>

class MainWindow;
class QLabel;
class QSlider;
class QToolBar;
class QToolButton;
class QWidget;

/**
 * @brief VideoMaker: iMovie-like "simple" mode on top of the Kdenlive main window.
 *
 * Simple mode hides menus, tool bars, status bar and dock title bars and adds
 * - a top bar (import, title, background, project name, light/dark, advanced, share),
 * - a tool bar above the project monitor (color, crop, stabilize, volume, speed, … → Kdenlive effects),
 * - a film bar above the timeline (cut, marker, undo, zoom).
 * QML reads vmMode.simple (e.g. the timeline hides its track headers).
 * Advanced mode is plain Kdenlive. The choice is stored as [VideoMaker] Erweitert.
 */
class SimpleMode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool simple READ isSimple NOTIFY simpleChanged)

public:
    static SimpleMode *instance();
    bool isSimple() const;
    /** @brief Build the bars and apply the stored mode; call once the main window UI is complete. */
    void attach(MainWindow *window);
    void setSimple(bool simple);
    /** @brief Append a bin clip at the end of the main video track (iMovie "+" / key E). */
    void appendToFilm(const QString &binId);

Q_SIGNALS:
    void simpleChanged(bool simple);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    explicit SimpleMode(QObject *parent = nullptr);
    QToolBar *buildTopBar();
    QWidget *buildAdjustBar();
    QWidget *buildFilmBar();
    QToolButton *toolButton(const QString &icon, const QString &tip, QWidget *parent);
    QToolButton *actionButton(const QString &actionName, const QString &text, QWidget *parent);
    void addEffectToSelection(const QString &effectId);
    void triggerAction(const QString &name);
    void showDock(int which);
    void setTheme(const QString &mode);
    void refreshIcons();
    void updateProjectName();
    void applyBinView(bool simple);

    QPointer<MainWindow> m_window;
    QToolBar *m_topBar{nullptr};
    QWidget *m_adjustBar{nullptr};
    QWidget *m_filmBar{nullptr};
    QLabel *m_projectName{nullptr};
    QToolButton *m_advancedButton{nullptr};
    QSlider *m_zoom{nullptr};
    QList<QToolButton *> m_iconButtons;
    bool m_simple{true};
};

/** @brief Line icon in the AMaker style, drawn in the current text color. */
QIcon vmIcon(const QString &name);
