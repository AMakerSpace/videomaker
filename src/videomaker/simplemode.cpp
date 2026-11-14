/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#include "simplemode.h"
#include "adjustpanel.h"
#include "amakertheme.h"

#include "core.h"
#include "kdenlivesettings.h"
#include "mainwindow.h"
#include "monitor/monitor.h"
#include "timeline2/view/timelinetabs.hpp"
#include "bin/bin.h"
#include "timeline2/model/timelineitemmodel.hpp"
#include "timeline2/view/timelinecontroller.h"
#include "timeline2/view/timelinewidget.h"

#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KToolBar>
#include <KSelectAction>
#include <KSharedConfig>
#include <QApplication>
#include <QButtonGroup>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QPainter>
#include <QSlider>
#include <QStatusBar>
#include <QSvgRenderer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

// Line icons on a 24 grid (same set as docs/ziel-imovie.html)
const QHash<QString, QString> &iconPaths()
{
    static const QHash<QString, QString> paths = {
        {QStringLiteral("text"), QStringLiteral("M5 5h14M12 5v15M9 20h6")},
        {QStringLiteral("balance"), QStringLiteral("M12 3a9 9 0 1 0 0 18zM12 3a9 9 0 0 1 0 18")},
        {QStringLiteral("palette"), QStringLiteral("M12 3a9 9 0 0 0 0 18c1.5 0 2-1 2-2s-1-2 0-3 2-1 3-1h1a3 3 0 0 0 3-3c0-5-4-9-9-9zM8 9h.01M12 7h.01M16 9h.01M8 13h.01")},
        {QStringLiteral("crop"), QStringLiteral("M6 2v16h16M2 6h16v16")},
        {QStringLiteral("stab"), QStringLiteral("M3 8h12v8H3zM15 11l5-3v8l-5-3M1 5h3M1 19h3")},
        {QStringLiteral("vol"), QStringLiteral("M4 9h4l5-4v14l-5-4H4zM16 9a4 4 0 0 1 0 6M19 6a8 8 0 0 1 0 12")},
        {QStringLiteral("eq"), QStringLiteral("M5 20V10M9 20V4M13 20v-8M17 20V7M21 20v-5")},
        {QStringLiteral("speed"), QStringLiteral("M4 18a9 9 0 1 1 16 0M12 14l4-5M12 14h.01")},
        {QStringLiteral("filter"), QStringLiteral("M9 9a5 5 0 1 0 0 .01M15 9a5 5 0 1 0 0 .01M12 15a5 5 0 1 0 0 .01")},
        {QStringLiteral("pip"), QStringLiteral("M3 5h18v14H3zM12 11h7v6h-7z")},
        {QStringLiteral("kf"), QStringLiteral("M12 3l9 9-9 9-9-9z")},
        {QStringLiteral("cc"), QStringLiteral("M3 5h18v14H3zM10 10a2 2 0 1 0 0 4M17 10a2 2 0 1 0 0 4")},
        {QStringLiteral("cut"), QStringLiteral("M6 6a2.5 2.5 0 1 0 0 .01M6 18a2.5 2.5 0 1 0 0 .01M8 7l12 11M8 17 20 6")},
        {QStringLiteral("mark"), QStringLiteral("M12 2v20M5 4h10l-2 3 2 3H5")},
        {QStringLiteral("undo"), QStringLiteral("M9 14 4 9l5-5M4 9h11a5 5 0 0 1 0 10h-3")},
        {QStringLiteral("redo"), QStringLiteral("M15 14l5-5-5-5M20 9H9a5 5 0 0 0 0 10h3")},
        {QStringLiteral("plus"), QStringLiteral("M12 4v16M4 12h16")},
    };
    return paths;
}

QString caps(const QString &text)
{
    return text.toUpper();
}

// Dock indices for showDock()
enum { DockMedia, DockEffects, DockTransitions, DockAdjust };

} // namespace

QIcon vmIcon(const QString &name)
{
    const QColor ink = QApplication::palette().color(QPalette::WindowText);
    const QString svg = QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'><path d='%1' fill='none' stroke='%2' stroke-width='2.4' "
                                       "stroke-linecap='square' stroke-linejoin='miter'/></svg>")
                            .arg(iconPaths().value(name), ink.name());
    QSvgRenderer renderer(svg.toUtf8());
    QIcon icon;
    for (int size : {16, 22, 32, 48}) {
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        renderer.render(&p);
        icon.addPixmap(pix);
    }
    return icon;
}

SimpleMode *SimpleMode::instance()
{
    static SimpleMode *mode = new SimpleMode(qApp);
    return mode;
}

SimpleMode::SimpleMode(QObject *parent)
    : QObject(parent)
{
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("VideoMaker"));
    m_simple = !cg.readEntry("Erweitert", false);
}

bool SimpleMode::isSimple() const
{
    return m_simple;
}

void SimpleMode::attach(MainWindow *window)
{
    m_window = window;
    // iMovie: E hängt die gewählten Medien ans Ende des Films
    auto *append = new QAction(i18n("Add to Movie"), window);
    append->setIcon(vmIcon(QStringLiteral("plus")));
    window->actionCollection()->addAction(QStringLiteral("vm_append_to_film"), append);
    window->actionCollection()->setDefaultShortcut(append, Qt::Key_E);
    connect(append, &QAction::triggered, this, [this]() {
        for (const QString &id : pCore->bin()->selectedClipsIds()) {
            appendToFilm(id);
        }
    });

    m_topBar = buildTopBar();
    window->addToolBar(Qt::TopToolBarArea, m_topBar);
    window->insertToolBarBreak(m_topBar);

    m_adjustPanel = new AdjustPanel(window);
    connect(m_adjustPanel, &AdjustPanel::showFullSettings, this, [this]() { showDock(DockAdjust); });
    m_adjustBar = buildAdjustBar();
    if (auto *lay = qobject_cast<QVBoxLayout *>(window->m_projectMonitor->layout())) {
        // Wie iMovie: Werkzeuge über der Vorschau, die Regler klappen direkt darunter auf
        lay->insertWidget(0, m_adjustBar);
        lay->insertWidget(1, m_adjustPanel);
    }
    m_filmBar = buildFilmBar();
    if (auto *lay = qobject_cast<QVBoxLayout *>(window->m_timelineToolBarContainer->layout())) {
        lay->insertWidget(0, m_filmBar);
    }

    connect(window, &QWidget::windowTitleChanged, this, &SimpleMode::updateProjectName);
    window->installEventFilter(this);
    AMakerTheme::watchSystem([](bool dark) { Q_EMIT pCore->switchDarkPalette(dark); });
    updateProjectName();
    setSimple(m_simple);
}

void SimpleMode::setSimple(bool simple)
{
    const bool changed = simple != m_simple;
    m_simple = simple;
    KConfigGroup cg(KSharedConfig::openConfig(), QStringLiteral("VideoMaker"));
    cg.writeEntry("Erweitert", !simple);
    if (!m_window) {
        return;
    }
    // Plain Kdenlive chrome only in advanced mode
    m_window->menuBar()->setVisible(!simple);
    m_window->statusBar()->setVisible(!simple);
    const auto bars = m_window->toolBars();
    for (KToolBar *bar : bars) {
        if (bar != m_topBar) {
            bar->setVisible(!simple);
        }
    }
    m_window->m_timelineToolBar->setVisible(!simple);
    m_topBar->setVisible(true);
    m_adjustBar->setVisible(simple);
    if (!simple) {
        m_adjustPanel->closePanel();
    }
    m_filmBar->setVisible(simple);
    applyBinView(simple);
    m_advancedButton->setChecked(!simple);
    Q_EMIT pCore->hideBars(simple || !KdenliveSettings::showtitlebars());
    if (changed) {
        Q_EMIT pCore->loadLayoutById(simple ? QStringLiteral("editing") : QStringLiteral("advanced"));
    }
    Q_EMIT simpleChanged(simple);
}

bool SimpleMode::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::PaletteChange) {
        refreshIcons();
    }
    return QObject::eventFilter(watched, event);
}

QToolButton *SimpleMode::toolButton(const QString &icon, const QString &tip, QWidget *parent)
{
    auto *b = new QToolButton(parent);
    b->setIcon(vmIcon(icon));
    b->setProperty("vmIcon", icon);
    b->setToolTip(tip);
    b->setAutoRaise(true);
    b->setIconSize(QSize(22, 22));
    m_iconButtons << b;
    return b;
}

QToolButton *SimpleMode::actionButton(const QString &actionName, const QString &text, QWidget *parent)
{
    auto *b = new QToolButton(parent);
    b->setText(caps(text));
    b->setToolButtonStyle(Qt::ToolButtonTextOnly);
    b->setProperty("vmButton", true);
    connect(b, &QToolButton::clicked, this, [this, actionName]() { triggerAction(actionName); });
    return b;
}

QToolBar *SimpleMode::buildTopBar()
{
    auto *bar = new QToolBar(i18n("VideoMaker"), m_window);
    bar->setObjectName(QStringLiteral("videomakerTopBar"));
    bar->setMovable(false);
    bar->setFloatable(false);
    bar->toggleViewAction()->setVisible(false);

    auto *w = new QWidget(bar);
    auto *lay = new QHBoxLayout(w);
    lay->setContentsMargins(6, 4, 6, 4);
    lay->setSpacing(10);
    lay->addWidget(actionButton(QStringLiteral("add_clip"), i18n("↓ Import"), w));
    lay->addWidget(actionButton(QStringLiteral("add_text_clip"), i18n("+ Title"), w));
    lay->addWidget(actionButton(QStringLiteral("add_color_clip"), i18n("+ Background"), w));
    lay->addStretch(1);
    m_projectName = new QLabel(w);
    m_projectName->setObjectName(QStringLiteral("vmProjectName"));
    QFont f = m_projectName->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() * 1.2);
    m_projectName->setFont(f);
    lay->addWidget(m_projectName);
    lay->addStretch(1);

    // Light / dark / auto as one segmented control
    auto *group = new QButtonGroup(w);
    const QString mode = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("VideoMaker")).readEntry("Thema", QStringLiteral("auto"));
    const QList<QPair<QString, QString>> modes = {{QStringLiteral("auto"), i18n("Auto")},
                                                  {QStringLiteral("hell"), i18n("Light")},
                                                  {QStringLiteral("dunkel"), i18n("Dark")}};
    for (const auto &m : modes) {
        auto *b = new QToolButton(w);
        b->setText(caps(m.second));
        b->setCheckable(true);
        b->setChecked(m.first == mode);
        b->setProperty("vmButton", true);
        group->addButton(b);
        lay->addWidget(b);
        connect(b, &QToolButton::clicked, this, [this, key = m.first]() { setTheme(key); });
    }
    group->setExclusive(true);

    m_advancedButton = new QToolButton(w);
    m_advancedButton->setText(caps(i18n("Advanced")));
    m_advancedButton->setToolTip(i18n("All tracks, menus and tools of Kdenlive"));
    m_advancedButton->setCheckable(true);
    m_advancedButton->setProperty("vmButton", true);
    connect(m_advancedButton, &QToolButton::clicked, this, [this](bool on) { setSimple(!on); });
    lay->addWidget(m_advancedButton);

    auto *share = actionButton(QStringLiteral("project_render"), i18n("Share ↗"), w);
    share->setProperty("vmPrimary", true);
    lay->addWidget(share);
    bar->addWidget(w);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return bar;
}

QWidget *SimpleMode::buildAdjustBar()
{
    auto *bar = new QWidget(m_window);
    bar->setObjectName(QStringLiteral("vmAdjustBar"));
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(6, 3, 6, 3);
    lay->setSpacing(2);
    auto *num = new QLabel(QStringLiteral("02"), bar);
    num->setObjectName(QStringLiteral("vmNumber"));
    lay->addWidget(num);
    lay->addSpacing(8);

    struct Tool {
        QString icon, tip, effect, action;
    };
    const QList<Tool> tools = {
        {QStringLiteral("text"), i18n("Title"), {}, QStringLiteral("add_text_clip")},
        {QStringLiteral("balance"), i18n("Color balance"), QStringLiteral("frei0r.colgate"), {}},
        {QStringLiteral("palette"), i18n("Color correction"), QStringLiteral("avfilter.eq"), {}},
        {QStringLiteral("crop"), i18n("Crop & Ken Burns"), QStringLiteral("qtcrop"), {}},
        {QStringLiteral("stab"), i18n("Stabilize"), QStringLiteral("vidstab"), {}},
        {QStringLiteral("vol"), i18n("Volume"), QStringLiteral("volume"), {}},
        {QStringLiteral("eq"), i18n("Noise & equalizer"), QStringLiteral("avfilter.equalizer"), {}},
        {QStringLiteral("speed"), i18n("Speed"), {}, QStringLiteral("edit_item_speed")},
        {QStringLiteral("filter"), i18n("Clip filters"), {}, QStringLiteral("#effects")},
        {{}, {}, {}, {}},
        {QStringLiteral("pip"), i18n("Picture in picture & green screen"), QStringLiteral("qtblend"), {}},
        {QStringLiteral("kf"), i18n("Keyframes"), {}, QStringLiteral("#adjust")},
        {QStringLiteral("cc"), i18n("Subtitles from speech"), {}, QStringLiteral("audio_recognition")},
    };
    for (const Tool &t : tools) {
        if (t.icon.isEmpty()) {
            auto *sep = new QWidget(bar);
            sep->setObjectName(QStringLiteral("vmSeparator"));
            sep->setFixedSize(3, 22);
            lay->addSpacing(6);
            lay->addWidget(sep);
            lay->addSpacing(6);
            continue;
        }
        auto *b = toolButton(t.icon, t.tip, bar);
        // Jedes Werkzeug ist auch eine Aktion (Tastenkürzel, DBus: …/actions/vm_tool_<name>)
        auto *act = new QAction(vmIcon(t.icon), t.tip, m_window);
        m_window->actionCollection()->addAction(QStringLiteral("vm_tool_") + t.icon, act);
        connect(b, &QToolButton::clicked, act, &QAction::trigger);
        connect(act, &QAction::triggered, this, [this, t]() {
            if (!t.effect.isEmpty()) {
                m_adjustPanel->toggle(t.effect, t.tip);
            } else if (t.action == QLatin1String("#effects")) {
                showDock(DockEffects);
            } else if (t.action == QLatin1String("#adjust")) {
                showDock(DockAdjust);
            } else {
                triggerAction(t.action);
            }
        });
        lay->addWidget(b);
    }
    lay->addStretch(1);
    return bar;
}

QWidget *SimpleMode::buildFilmBar()
{
    auto *bar = new QWidget(m_window);
    bar->setObjectName(QStringLiteral("vmFilmBar"));
    auto *lay = new QHBoxLayout(bar);
    lay->setContentsMargins(6, 3, 10, 3);
    lay->setSpacing(4);
    auto *num = new QLabel(QStringLiteral("03"), bar);
    num->setObjectName(QStringLiteral("vmNumber"));
    lay->addWidget(num);
    auto *title = new QLabel(caps(i18n("Movie")), bar);
    title->setObjectName(QStringLiteral("vmBoxTitle"));
    lay->addWidget(title);
    lay->addSpacing(10);
    const QList<QPair<QString, QString>> tools = {{QStringLiteral("plus"), QStringLiteral("vm_append_to_film")},
                                                  {QStringLiteral("cut"), QStringLiteral("cut_timeline_clip")},
                                                  {QStringLiteral("mark"), QStringLiteral("add_marker_guide_quickly")},
                                                  {QStringLiteral("undo"), QStringLiteral("edit_undo")},
                                                  {QStringLiteral("redo"), QStringLiteral("edit_redo")}};
    for (const auto &t : tools) {
        QAction *a = m_window->actionCollection()->action(t.second);
        auto *b = toolButton(t.first, a ? a->toolTip() : t.second, bar);
        connect(b, &QToolButton::clicked, this, [this, name = t.second]() { triggerAction(name); });
        lay->addWidget(b);
    }
    lay->addStretch(1);
    auto *zoomLabel = new QLabel(caps(i18n("Zoom")), bar);
    zoomLabel->setObjectName(QStringLiteral("vmBoxTitle"));
    lay->addWidget(zoomLabel);
    // Mirror of Kdenlive's zoom slider (which lives in the hidden status bar); left = closer, like iMovie
    m_zoom = new QSlider(Qt::Horizontal, bar);
    m_zoom->setRange(m_window->m_zoomSlider->minimum(), m_window->m_zoomSlider->maximum());
    m_zoom->setInvertedAppearance(true);
    m_zoom->setValue(m_window->m_zoomSlider->value());
    m_zoom->setFixedWidth(140);
    connect(m_zoom, &QSlider::valueChanged, m_window->m_zoomSlider, &QSlider::setValue);
    connect(m_window->m_zoomSlider, &QSlider::valueChanged, m_zoom, [this](int v) {
        const QSignalBlocker block(m_zoom);
        m_zoom->setValue(v);
    });
    lay->addWidget(m_zoom);
    return bar;
}

void SimpleMode::appendToFilm(const QString &binId)
{
    TimelineWidget *timeline = m_window ? m_window->getCurrentTimeline() : nullptr;
    if (!timeline || binId.isEmpty()) {
        return;
    }
    TimelineController *ctl = timeline->controller();
    const int track = ctl->getModel()->getLowestVideoTrackIndex();
    const int pos = ctl->duration();
    const int clipId = ctl->insertClip(track, pos, binId, true, true, true);
    if (clipId > -1) {
        // Abspielkopf ans neue Ende, ganzen Film einpassen (wie iMovie)
        ctl->setPosition(pos);
        m_window->slotFitZoom();
    }
}

void SimpleMode::applyBinView(bool simple)
{
    // Einfach: große Kacheln wie iMovie, Erweitert: Kdenlives Baumansicht
    auto *mode = qobject_cast<KSelectAction *>(m_window->actionCollection()->action(QStringLiteral("bin_view_mode")));
    if (!mode) {
        return;
    }
    // Werte aus Bin::BinViewType (privat): 0 = Baum, 1 = Kacheln
    const int wanted = simple ? 1 : 0;
    const auto actions = mode->actions();
    for (QAction *a : actions) {
        if (a->data().toInt() == wanted && mode->currentAction() != a) {
            mode->setCurrentAction(a);
            Q_EMIT mode->actionTriggered(a);
        }
    }
    if (simple) {
        // Größte Kacheln über den Zoom-Regler der Mediathek (er speichert den Wert auch im Projekt)
        if (auto *zoom = pCore->bin()->findChild<QSlider *>()) {
            zoom->setValue(zoom->maximum());
        }
    }
}

void SimpleMode::triggerAction(const QString &name)
{
    if (QAction *a = m_window->actionCollection()->action(name)) {
        a->trigger();
    }
}

void SimpleMode::showDock(int which)
{
    KDDockWidgets::QtWidgets::DockWidget *dock = nullptr;
    switch (which) {
    case DockMedia:
        dock = m_window->m_projectBinDock;
        break;
    case DockEffects:
        dock = m_window->m_effectListDock;
        break;
    case DockTransitions:
        dock = m_window->m_compositionListDock;
        break;
    default:
        dock = m_window->m_effectStackDock;
        break;
    }
    if (dock) {
        dock->open();
        dock->setAsCurrentTab();
    }
}

void SimpleMode::setTheme(const QString &mode)
{
    AMakerTheme::setMode(mode);
    Q_EMIT pCore->switchDarkPalette(AMakerTheme::isDark());
}

void SimpleMode::refreshIcons()
{
    for (QToolButton *b : std::as_const(m_iconButtons)) {
        b->setIcon(vmIcon(b->property("vmIcon").toString()));
    }
}

void SimpleMode::updateProjectName()
{
    if (!m_projectName || !m_window) {
        return;
    }
    // Caption is "<project>[*] — VideoMaker"; show just the project
    QString name = m_window->windowTitle();
    name.remove(QStringLiteral("[*]"));
    const int dash = name.lastIndexOf(QStringLiteral(" — "));
    if (dash > 0) {
        name.truncate(dash);
    }
    m_projectName->setText(name.trimmed());
}
