/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#include "adjustpanel.h"

#include "assets/model/assetcommand.hpp"
#include "assets/model/assetparametermodel.hpp"
#include "core.h"
#include "effects/effectstack/model/effectstackmodel.hpp"
#include "mainwindow.h"
#include "timeline2/model/timelineitemmodel.hpp"
#include "timeline2/view/timelinecontroller.h"
#include "timeline2/view/timelinewidget.h"

#include <KLocalizedString>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {

constexpr int Steps = 1000;
constexpr int MaxSliders = 4;

TimelineController *controller()
{
    TimelineWidget *timeline = pCore->window() ? pCore->window()->getCurrentTimeline() : nullptr;
    return timeline ? timeline->controller() : nullptr;
}

// Animated values look like "00:00:00.000=0.5;…" – a single keyframe is treated like a plain number
bool singleValue(const QString &value)
{
    return !value.contains(QLatin1Char(';'));
}

QString valuePrefix(const QString &value)
{
    const int eq = value.indexOf(QLatin1Char('='));
    return eq < 0 ? QString() : value.left(eq + 1);
}

double numberOf(const QString &value)
{
    const int eq = value.indexOf(QLatin1Char('='));
    return (eq < 0 ? value : value.mid(eq + 1)).toDouble();
}

} // namespace

AdjustPanel::AdjustPanel(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("vmAdjustPanel"));
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(10, 6, 10, 8);
    outer->setSpacing(6);

    auto *head = new QHBoxLayout;
    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName(QStringLiteral("vmBoxTitle"));
    head->addWidget(m_titleLabel);
    head->addStretch(1);
    auto *full = new QPushButton(i18n("All settings"), this);
    connect(full, &QPushButton::clicked, this, &AdjustPanel::showFullSettings);
    head->addWidget(full);
    auto *close = new QPushButton(QStringLiteral("✕"), this);
    close->setToolTip(i18n("Close"));
    connect(close, &QPushButton::clicked, this, &AdjustPanel::closePanel);
    head->addWidget(close);
    outer->addLayout(head);

    m_rows = new QVBoxLayout;
    m_rows->setSpacing(4);
    outer->addLayout(m_rows);
    hide();
}

void AdjustPanel::toggle(const QString &effectId, const QString &title)
{
    if (isVisible() && effectId == m_effectId) {
        closePanel();
        return;
    }
    m_effectId = effectId;
    m_title = title;
    m_titleLabel->setText(title.toUpper());
    if (TimelineController *ctl = controller()) {
        // Neu aufbauen, wenn ein anderer Clip gewählt wird
        disconnect(ctl, &TimelineController::selectionChanged, this, nullptr);
        connect(ctl, &TimelineController::selectionChanged, this, [this]() {
            if (isVisible()) {
                rebuild();
            }
        });
    }
    rebuild();
    show();
}

void AdjustPanel::closePanel()
{
    hide();
    clearRows();
    m_effectId.clear();
    Q_EMIT closed();
}

void AdjustPanel::clearRows()
{
    while (QLayoutItem *item = m_rows->takeAt(0)) {
        if (QLayout *l = item->layout()) {
            while (QLayoutItem *child = l->takeAt(0)) {
                delete child->widget();
                delete child;
            }
        }
        delete item->widget();
        delete item;
    }
}

void AdjustPanel::setMessage(const QString &text)
{
    auto *label = new QLabel(text, this);
    label->setWordWrap(true);
    m_rows->addWidget(label);
}

void AdjustPanel::rebuild()
{
    clearRows();
    TimelineController *ctl = controller();
    const int mainClip = ctl ? ctl->getMainSelectedClip() : -1;
    if (mainClip < 0) {
        setMessage(i18n("Select a clip in the movie first."));
        return;
    }
    // Ein Videoclip besteht aus Bild- und Tonteil: den Teil nehmen, zu dem das Werkzeug passt
    std::vector<int> candidates = {mainClip};
    for (int id : ctl->getModel()->getCurrentSelection()) {
        if (id != mainClip && ctl->getModel()->isClip(id)) {
            candidates.push_back(id);
        }
    }
    std::shared_ptr<AssetParameterModel> model;
    for (int id : candidates) {
        std::shared_ptr<EffectStackModel> stack = ctl->getModel()->getClipEffectStackModel(id);
        if (stack && (model = stack->getAssetModelById(m_effectId))) {
            break;
        }
    }
    for (int id : candidates) {
        if (model) {
            break;
        }
        std::shared_ptr<EffectStackModel> stack = ctl->getModel()->getClipEffectStackModel(id);
        if (stack && stack->appendEffect(m_effectId, true)) {
            model = stack->getAssetModelById(m_effectId);
        }
    }
    if (!model) {
        setMessage(i18n("This tool cannot be used on the selected clip."));
        return;
    }
    int shown = 0;
    for (int row = 0; row < model->rowCount() && shown < MaxSliders; ++row) {
        const QModelIndex ix = model->index(row, 0);
        const auto type = model->data(ix, AssetParameterModel::TypeRole).value<ParamType>();
        const QString value = model->data(ix, AssetParameterModel::ValueRole).toString();
        if (type == ParamType::Double || (type == ParamType::KeyframeParam && singleValue(value))) {
            addSlider(model, row);
            ++shown;
        }
    }
    if (shown == 0) {
        setMessage(i18n("This tool has no simple sliders. Use “All settings”."));
    }
}

void AdjustPanel::addSlider(const std::shared_ptr<AssetParameterModel> &model, int row)
{
    const QModelIndex ix = model->index(row, 0);
    const double min = model->data(ix, AssetParameterModel::MinRole).toDouble();
    const double max = model->data(ix, AssetParameterModel::MaxRole).toDouble();
    double factor = model->data(ix, AssetParameterModel::FactorRole).toDouble();
    if (factor == 0.) {
        factor = 1.;
    }
    const double def = model->data(ix, AssetParameterModel::DefaultRole).toDouble();
    const int decimals = model->data(ix, AssetParameterModel::DecimalsRole).toInt();
    const QString suffix = model->data(ix, AssetParameterModel::SuffixRole).toString();
    if (max <= min) {
        return;
    }

    auto *line = new QHBoxLayout;
    auto *name = new QLabel(model->data(ix, Qt::DisplayRole).toString(), this);
    name->setMinimumWidth(140);
    auto *slider = new QSlider(Qt::Horizontal, this);
    slider->setRange(0, Steps);
    auto *number = new QLabel(this);
    number->setMinimumWidth(70);
    number->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *reset = new QPushButton(QStringLiteral("↺"), this);
    reset->setToolTip(i18n("Reset"));
    line->addWidget(name);
    line->addWidget(slider, 1);
    line->addWidget(number);
    line->addWidget(reset);
    m_rows->addLayout(line);

    const QString prefix = valuePrefix(model->data(ix, AssetParameterModel::ValueRole).toString());
    // Angezeigter Wert = gespeicherter Wert × Faktor (wie in Kdenlives eigenen Reglern)
    auto toSlider = [=](double shownValue) { return qRound((shownValue - min) / (max - min) * Steps); };
    auto fromSlider = [=](int pos) { return min + (max - min) * pos / Steps; };
    auto showNumber = [=](double shownValue) { number->setText(QString::number(shownValue, 'f', qBound(0, decimals, 3)) + suffix); };

    const double current = numberOf(model->data(ix, AssetParameterModel::ValueRole).toString()) * factor;
    slider->setValue(toSlider(current));
    showNumber(current);

    std::weak_ptr<AssetParameterModel> weak = model;
    auto commit = [=](double shownValue) {
        auto m = weak.lock();
        if (!m) {
            return;
        }
        const QModelIndex index = m->index(row, 0);
        const QString stored = prefix + QString::number(shownValue / factor, 'g', 8);
        // AssetCommand fasst schnelle Änderungen am selben Regler zu einem Rückgängig-Schritt zusammen
        pCore->pushUndo(new AssetCommand(m, index, stored));
        showNumber(shownValue);
    };
    connect(slider, &QSlider::valueChanged, this, [=](int pos) { commit(fromSlider(pos)); });
    connect(reset, &QPushButton::clicked, this, [=]() {
        const QSignalBlocker block(slider);
        slider->setValue(toSlider(def));
        commit(def);
    });
}
