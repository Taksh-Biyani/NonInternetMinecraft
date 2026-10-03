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

#include <QWizard>

#include "offline/BundleExportTask.h"

class BaseInstance;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class VersionSelectWidget;

// File → Export Offline Bundle… (Versions mode) and an instance's Export → Offline Bundle… (Instance mode, with that
// instance preselected). Three steps: what to bundle, review, build (spec §5.2).
class ExportBundleWizard : public QWizard {
    Q_OBJECT
   public:
    explicit ExportBundleWizard(QWidget* parent, BaseInstance* instance = nullptr);

    OfflineBundle::ExportRequest request() const;
    void reject() override;

   private:
    void buildWhatPage(BaseInstance* instance);
    void buildReviewPage();
    void buildBuildPage();
    void updateLoaderList();
    void updateWorldsSize();
    void addCurrentSet();
    void refreshReview();
    void startBuild();

    // What
    QButtonGroup* m_mode = nullptr;
    QStackedWidget* m_modeStack = nullptr;
    VersionSelectWidget* m_mcList = nullptr;
    QCheckBox* m_snapshots = nullptr;
    QComboBox* m_loader = nullptr;
    VersionSelectWidget* m_loaderList = nullptr;
    QListWidget* m_sets = nullptr;
    QList<OfflineBundle::ExportSet> m_setList;
    QComboBox* m_instance = nullptr;
    QCheckBox* m_worlds = nullptr;
    QListWidget* m_extraJava = nullptr;
    // Review
    QLabel* m_summary = nullptr;
    QLineEdit* m_destination = nullptr;
    // Build
    QLabel* m_buildStatus = nullptr;
    QProgressBar* m_buildProgress = nullptr;
    QLabel* m_buildResult = nullptr;
    QPushButton* m_showInFolder = nullptr;
    Task::Ptr m_task;
    bool m_buildDone = false;
    bool m_closeWhenStopped = false;
};
