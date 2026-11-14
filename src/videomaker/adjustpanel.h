/*
    SPDX-FileCopyrightText: 2026 AMakerSpace
    SPDX-License-Identifier: GPL-3.0-only
*/

#pragma once

#include <QFrame>
#include <QPointer>

#include <memory>

class AssetParameterModel;
class QLabel;
class QVBoxLayout;

/**
 * @brief VideoMaker: iMovie-like inspector strip under the viewer tools.
 *
 * Shows the numeric parameters of one effect on the selected timeline clip as sliders.
 * The effect is added when the tool is opened; slider changes go through AssetCommand (undoable).
 * Parameters that are not plain numbers (rectangles, curves, several keyframes) are left
 * to Kdenlive's full "Adjust" panel.
 */
class AdjustPanel : public QFrame
{
    Q_OBJECT

public:
    explicit AdjustPanel(QWidget *parent = nullptr);
    /** @brief Open the panel for @p effectId with title @p title (closes it when already open for this effect). */
    void toggle(const QString &effectId, const QString &title);
    void closePanel();

Q_SIGNALS:
    void showFullSettings();
    void closed();

private:
    void rebuild();
    void clearRows();
    void addSlider(const std::shared_ptr<AssetParameterModel> &model, int row);
    void setMessage(const QString &text);

    QString m_effectId;
    QString m_title;
    QLabel *m_titleLabel{nullptr};
    QVBoxLayout *m_rows{nullptr};
};
