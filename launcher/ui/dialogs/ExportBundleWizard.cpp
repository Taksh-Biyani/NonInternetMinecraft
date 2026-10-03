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

#include "ExportBundleWizard.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>
#include <QWizardPage>
#include <functional>

#include "Application.h"
#include "BaseVersionList.h"
#include "FileSystem.h"
#include "Filter.h"
#include "InstanceList.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/MinecraftInstance.h"
#include "offline/BundleMessages.h"
#include "offline/ExportRules.h"
#include "offline/OfflineMode.h"
#include "offline/RemovableDrives.h"
#include "ui/dialogs/FriendlyErrorDialog.h"
#include "ui/widgets/GuideButton.h"
#include "ui/widgets/VersionSelectWidget.h"

using namespace OfflineBundle;

namespace {
enum Page { WhatPage, ReviewPage, BuildPage };
enum Mode { VersionsMode, InstanceMode };

// A page whose Next/Finish button follows a function, so the wizard needs no page subclasses with Q_OBJECT.
class FuncPage : public QWizardPage {
   public:
    std::function<bool()> complete = [] { return true; };
    std::function<void()> onEnter;
    bool isComplete() const override { return complete(); }
    void initializePage() override
    {
        if (onEnter)
            onEnter();
    }
    void refresh() { emit completeChanged(); }
};

qint64 folderSize(const QString& dir)
{
    qint64 total = 0;
    QDirIterator it(dir, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        total += it.fileInfo().size();
    }
    return total;
}

QString defaultFolder()
{
    const QString drive = firstRemovableDriveRoot();
    return drive.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) : drive;
}
}  // namespace

ExportBundleWizard::ExportBundleWizard(QWidget* parent, BaseInstance* instance) : QWizard(parent)
{
    setWindowTitle(tr("Export offline bundle"));
    setWizardStyle(QWizard::ModernStyle);
    setOption(QWizard::NoBackButtonOnLastPage);
    setOption(QWizard::HaveHelpButton, true);
    connect(this, &QWizard::helpRequested, this, [this] { GuideButton::openSection(OfflineGuide::Section::MakeBundle, this); });
    setMinimumSize(720, 560);
    buildWhatPage(instance);
    buildReviewPage();
    buildBuildPage();
}

void ExportBundleWizard::buildWhatPage(BaseInstance* preselected)
{
    auto* page = new FuncPage;
    page->setTitle(tr("What to bundle"));
    page->setSubTitle(tr("Everything you pick is downloaded now and packed into one file that installs on a computer without internet."));
    auto* layout = new QVBoxLayout(page);

    auto* versionsRadio = new QRadioButton(tr("Minecraft versions and mod loaders"));
    auto* instanceRadio = new QRadioButton(tr("An instance, with its mods and settings"));
    m_mode = new QButtonGroup(page);
    m_mode->addButton(versionsRadio, VersionsMode);
    m_mode->addButton(instanceRadio, InstanceMode);
    layout->addWidget(versionsRadio);
    layout->addWidget(instanceRadio);

    m_modeStack = new QStackedWidget;
    layout->addWidget(m_modeStack, 1);

    // Versions mode
    auto* versions = new QWidget;
    auto* vLayout = new QVBoxLayout(versions);
    auto* pickers = new QHBoxLayout;
    auto* mcColumn = new QVBoxLayout;
    mcColumn->addWidget(new QLabel(tr("Minecraft version:")));
    m_mcList = new VersionSelectWidget(versions);
    m_mcList->initialize(APPLICATION->metadataIndex()->get("net.minecraft").get());
    m_mcList->setFilter(BaseVersionList::TypeRole, Filters::regexp(QRegularExpression("(release)")));
    m_mcList->selectRecommended();
    mcColumn->addWidget(m_mcList);
    m_snapshots = new QCheckBox(tr("Show snapshots"));
    mcColumn->addWidget(m_snapshots);
    pickers->addLayout(mcColumn);
    auto* loaderColumn = new QVBoxLayout;
    loaderColumn->addWidget(new QLabel(tr("Mod loader:")));
    m_loader = new QComboBox;
    m_loader->addItem(tr("None (vanilla)"), QString());
    for (const QString& uid : { "net.neoforged", "net.minecraftforge", "net.fabricmc.fabric-loader", "org.quiltmc.quilt-loader",
                                "com.mumfrey.liteloader" })
        m_loader->addItem(loaderDisplayName(uid), uid);
    loaderColumn->addWidget(m_loader);
    m_loaderList = new VersionSelectWidget(versions);
    loaderColumn->addWidget(m_loaderList);
    pickers->addLayout(loaderColumn);
    vLayout->addLayout(pickers, 1);
    auto* addButton = new QPushButton(tr("Add to bundle"));
    addButton->setToolTip(tr("Adds the selected Minecraft version (and loader) to the list below. You can add several."));
    vLayout->addWidget(addButton, 0, Qt::AlignLeft);
    vLayout->addWidget(new QLabel(tr("In this bundle:")));
    m_sets = new QListWidget;
    m_sets->setMaximumHeight(90);
    vLayout->addWidget(m_sets);
    auto* removeButton = new QPushButton(tr("Remove"));
    vLayout->addWidget(removeButton, 0, Qt::AlignLeft);
    m_modeStack->addWidget(versions);

    // Instance mode
    auto* instancePanel = new QWidget;
    auto* iLayout = new QVBoxLayout(instancePanel);
    iLayout->addWidget(new QLabel(tr("Instance:")));
    m_instance = new QComboBox;
    auto* instances = APPLICATION->instances();
    for (int i = 0; i < instances->count(); ++i) {
        auto* inst = instances->at(i);
        if (dynamic_cast<MinecraftInstance*>(inst))
            m_instance->addItem(inst->name(), inst->id());
    }
    iLayout->addWidget(m_instance);
    m_worlds = new QCheckBox;
    m_worlds->setToolTip(tr("Worlds can be large. Leave this off to share only the modpack."));
    iLayout->addWidget(m_worlds);
    iLayout->addStretch();
    m_modeStack->addWidget(instancePanel);

    // Java (both modes)
    layout->addWidget(new QLabel(tr("Java: the version each Minecraft version needs is added automatically. "
                                    "Tick any other Java below to add it too:")));
    m_extraJava = new QListWidget;
    m_extraJava->setMaximumHeight(70);
    const QDir javaDir(QDir::current().absoluteFilePath(APPLICATION->javaPath()));
    for (const QString& folder : javaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        auto* item = new QListWidgetItem(folder, m_extraJava);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
    }
    layout->addWidget(m_extraJava);

    // Behaviour
    connect(m_mode, &QButtonGroup::idClicked, this, [this, page](int id) {
        m_modeStack->setCurrentIndex(id);
        page->refresh();
    });
    connect(m_snapshots, &QCheckBox::toggled, this, [this](bool on) {
        m_mcList->setFilter(BaseVersionList::TypeRole, Filters::regexp(QRegularExpression(on ? "(release)|(snapshot)" : "(release)")));
    });
    connect(m_mcList, &VersionSelectWidget::selectedVersionChanged, this, [this] { updateLoaderList(); });
    connect(m_loader, &QComboBox::currentIndexChanged, this, [this] { updateLoaderList(); });
    connect(addButton, &QPushButton::clicked, this, [this, page] {
        addCurrentSet();
        page->refresh();
    });
    connect(removeButton, &QPushButton::clicked, this, [this, page] {
        const int row = m_sets->currentRow();
        if (row >= 0) {
            delete m_sets->takeItem(row);
            m_setList.removeAt(row);
            page->refresh();
        }
    });
    connect(m_instance, &QComboBox::currentIndexChanged, this, [this] { updateWorldsSize(); });
    page->complete = [this] { return m_mode->checkedId() == InstanceMode ? m_instance->currentIndex() >= 0 : !m_setList.isEmpty(); };

    const bool instanceMode = preselected != nullptr;
    (instanceMode ? instanceRadio : versionsRadio)->setChecked(true);
    m_modeStack->setCurrentIndex(instanceMode ? InstanceMode : VersionsMode);
    if (preselected)
        m_instance->setCurrentIndex(m_instance->findData(preselected->id()));
    updateWorldsSize();
    updateLoaderList();
    setPage(WhatPage, page);
}

void ExportBundleWizard::updateLoaderList()
{
    const QString uid = m_loader->currentData().toString();
    auto mc = m_mcList->selectedVersion();
    m_loaderList->setEnabled(!uid.isEmpty() && mc);
    if (uid.isEmpty() || !mc) {
        m_loaderList->setExactFilter(BaseVersionList::ParentVersionRole, "AAA");  // empty list
        return;
    }
    m_loaderList->initialize(APPLICATION->metadataIndex()->get(uid).get());
    // Fabric and Quilt versions aren't tied to one Minecraft version (same rule as the Add Instance page).
    const bool anyVersion = uid == "net.fabricmc.fabric-loader" || uid == "org.quiltmc.quilt-loader";
    m_loaderList->setExactFilter(BaseVersionList::ParentVersionRole, anyVersion ? QString() : mc->descriptor());
    m_loaderList->selectRecommended();
}

void ExportBundleWizard::addCurrentSet()
{
    auto mc = m_mcList->selectedVersion();
    if (!mc)
        return;
    ExportSet set{ mc->descriptor(), m_loader->currentData().toString(), {} };
    QString label = tr("Minecraft %1").arg(set.minecraft);
    if (!set.loaderUid.isEmpty()) {
        auto loader = m_loaderList->selectedVersion();
        if (!loader)
            return;
        set.loaderVersion = loader->descriptor();
        label += QString(" + %1 %2").arg(loaderDisplayName(set.loaderUid), set.loaderVersion);
    }
    for (const ExportSet& existing : m_setList)
        if (existing.minecraft == set.minecraft && existing.loaderUid == set.loaderUid && existing.loaderVersion == set.loaderVersion)
            return;
    m_setList.append(set);
    m_sets->addItem(label);
}

void ExportBundleWizard::updateWorldsSize()
{
    auto* inst = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->getInstanceById(m_instance->currentData().toString()));
    const qint64 size = inst ? folderSize(FS::PathCombine(inst->gameRoot(), "saves")) : 0;
    m_worlds->setText(tr("Include worlds (saves), %1").arg(Messages::formatSize(size)));
}

void ExportBundleWizard::buildReviewPage()
{
    auto* page = new FuncPage;
    page->setTitle(tr("Review"));
    page->setSubTitle(tr("Check what goes into the bundle and where it's saved."));
    page->setCommitPage(true);
    page->setButtonText(QWizard::CommitButton, tr("Build bundle"));
    auto* layout = new QVBoxLayout(page);
    m_summary = new QLabel;
    m_summary->setWordWrap(true);
    m_summary->setTextFormat(Qt::RichText);
    layout->addWidget(m_summary, 1);
    layout->addWidget(new QLabel(tr("Save the bundle as:")));
    auto* row = new QHBoxLayout;
    m_destination = new QLineEdit;
    auto* browse = new QPushButton(tr("Browse..."));
    row->addWidget(m_destination, 1);
    row->addWidget(browse);
    layout->addLayout(row);
    connect(browse, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, tr("Save offline bundle"), m_destination->text(), tr("Offline bundles (*.zip)"));
        if (!path.isEmpty())
            m_destination->setText(path);
    });
    connect(m_destination, &QLineEdit::textChanged, page, &FuncPage::refresh);
    page->complete = [this] { return m_destination->text().endsWith(".zip", Qt::CaseInsensitive); };
    page->onEnter = [this] { refreshReview(); };
    setPage(ReviewPage, page);
}

void ExportBundleWizard::refreshReview()
{
    QString list;
    QString fileName;
    const bool instanceMode = m_mode->checkedId() == InstanceMode;
    if (instanceMode) {
        auto* inst = APPLICATION->instances()->getInstanceById(m_instance->currentData().toString());
        list += "<li>" + tr("Instance: %1").arg(m_instance->currentText().toHtmlEscaped()) + "</li>";
        if (m_worlds->isChecked())
            list += "<li>" + tr("With its worlds") + "</li>";
        const qint64 size = inst ? folderSize(inst->instanceRoot()) : 0;
        list += "<li>" + tr("Plus the Minecraft version, loader and Java it needs (instance folder: %1)").arg(Messages::formatSize(size)) + "</li>";
        fileName = suggestedInstanceFileName(m_instance->currentText());
    } else {
        QList<ComponentEntry> components;
        for (int i = 0; i < m_sets->count(); ++i) {
            list += "<li>" + m_sets->item(i)->text().toHtmlEscaped() + "</li>";
            const ExportSet& set = m_setList.at(i);
            components.append({ "net.minecraft", set.minecraft, "Minecraft" });
            if (!set.loaderUid.isEmpty())
                components.append({ set.loaderUid, set.loaderVersion, loaderDisplayName(set.loaderUid) });
        }
        list += "<li>" + tr("The Java each version needs") + "</li>";
        fileName = suggestedFileName(components);
    }
    for (int i = 0; i < m_extraJava->count(); ++i)
        if (m_extraJava->item(i)->checkState() == Qt::Checked)
            list += "<li>" + tr("Java folder: %1").arg(m_extraJava->item(i)->text().toHtmlEscaped()) + "</li>";
    const QString guidance = tr("Expect about 0.6 GB for one Minecraft version with its Java. Each extra loader for the same version adds "
                                "only 5-40 MB; another Minecraft version adds about 0.5 GB.");
    m_summary->setText(tr("<p>The bundle will contain:</p><ul>%1</ul><p>%2</p>").arg(list, guidance.toHtmlEscaped()));
    m_destination->setText(QDir(defaultFolder()).absoluteFilePath(fileName));
}

void ExportBundleWizard::buildBuildPage()
{
    auto* page = new FuncPage;
    page->setTitle(tr("Build"));
    page->setSubTitle(tr("Downloading and packing. This can take a while; you can keep using the launcher."));
    auto* layout = new QVBoxLayout(page);
    m_buildStatus = new QLabel;
    m_buildStatus->setWordWrap(true);
    layout->addWidget(m_buildStatus);
    m_buildProgress = new QProgressBar;
    layout->addWidget(m_buildProgress);
    m_buildResult = new QLabel;
    m_buildResult->setWordWrap(true);
    layout->addWidget(m_buildResult);
    m_showInFolder = new QPushButton(tr("Show in folder"));
    m_showInFolder->hide();
    layout->addWidget(m_showInFolder, 0, Qt::AlignLeft);
    layout->addStretch();
    connect(m_showInFolder, &QPushButton::clicked, this,
            [this] { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_destination->text()).absolutePath())); });
    page->complete = [this] { return m_buildDone; };
    page->onEnter = [this] { startBuild(); };
    setPage(BuildPage, page);
}

OfflineBundle::ExportRequest ExportBundleWizard::request() const
{
    ExportRequest request;
    request.outputPath = m_destination->text();
    if (m_mode->checkedId() == InstanceMode) {
        request.kind = Kind::Instance;
        request.instanceId = m_instance->currentData().toString();
        request.includeWorlds = m_worlds->isChecked();
    } else {
        request.sets = m_setList;
    }
    for (int i = 0; i < m_extraJava->count(); ++i)
        if (m_extraJava->item(i)->checkState() == Qt::Checked)
            request.extraJavaFolders.append(m_extraJava->item(i)->text());
    return request;
}

void ExportBundleWizard::startBuild()
{
    // "Auto" offline mode counts as offline until the startup network check has answered, and the export refuses to run
    // offline, so wait for the check first (same as the --export-bundle command line).
    if (auto* mode = APPLICATION->offlineMode(); mode && mode->awaitingCheck()) {
        m_buildStatus->setText(tr("Checking the internet connection..."));
        connect(mode, &OfflineMode::checkStateChanged, this, &ExportBundleWizard::startBuild, Qt::SingleShotConnection);
        return;
    }
    auto* page = static_cast<FuncPage*>(this->page(BuildPage));
    auto task = makeShared<ExportTask>(request());
    m_task = task;
    connect(task.get(), &Task::status, m_buildStatus, &QLabel::setText);
    connect(task.get(), &Task::progress, this, [this](qint64 current, qint64 total) {
        m_buildProgress->setMaximum(total > 0 ? int(qMin<qint64>(total, INT_MAX)) : 0);
        m_buildProgress->setValue(int(qMin<qint64>(current, INT_MAX)));
    });
    connect(task.get(), &Task::succeeded, this, [this, task, page] {
        m_buildResult->setText(tr("Done! The bundle is saved as %1 (%2). Copy it to the offline computer and use Import Bundle there.")
                                   .arg(QDir::toNativeSeparators(m_destination->text()), Messages::formatSize(task->bundleSize())));
        m_buildProgress->setMaximum(1);
        m_buildProgress->setValue(1);
        m_showInFolder->show();
        m_buildDone = true;
        page->refresh();
    });
    connect(task.get(), &Task::failed, this, [this, task, page](const QString& reason) {
        m_buildResult->setText(tr("The bundle wasn't made."));
        m_buildDone = true;
        page->refresh();
        if (m_closeWhenStopped) {
            QWizard::reject();
            return;
        }
        FriendlyErrorDialog::show(this, tr("Couldn't make the bundle"), reason, task->errorDetails(), OfflineGuide::Section::MakeBundle);
    });
    connect(task.get(), &Task::aborted, this, [this, page] {
        m_buildResult->setText(tr("Cancelled. Nothing was saved."));
        m_buildDone = true;
        page->refresh();
        if (m_closeWhenStopped)
            QWizard::reject();
    });
    task->start();
}

void ExportBundleWizard::reject()
{
    if (m_task && m_task->isRunning()) {
        m_closeWhenStopped = true;
        m_buildStatus->setText(tr("Cancelling..."));
        m_task->abort();
        return;
    }
    QWizard::reject();
}
