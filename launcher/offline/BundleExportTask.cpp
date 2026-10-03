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

#include "BundleExportTask.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStorageInfo>
#include <QThreadPool>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent>

#include "Application.h"
#include "BuildConfig.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "meta/Index.h"
#include "meta/VersionList.h"
#include "minecraft/Component.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/MinecraftLoadAndCheck.h"
#include "minecraft/PackProfile.h"
#include "minecraft/update/AssetUpdateTask.h"
#include "minecraft/update/ElyPatchTask.h"
#include "minecraft/update/LibrariesTask.h"
#include "offline/BundleMessages.h"
#include "offline/ExportJavaTask.h"
#include "offline/ForgePrepTask.h"
#include "offline/OfflineMode.h"
#include "settings/INISettingsObject.h"

namespace OfflineBundle {

static const QString s_forgeWrapperMain = "io.github.zekerzhayard.forgewrapper.installer.Main";

ExportTask::ExportTask(ExportRequest request) : m_request(std::move(request))
{
    m_context = windowsX64Context();
    connect(&m_watcher, &QFutureWatcher<WriteResult>::finished, this, &ExportTask::finishWriting);
}

ExportTask::~ExportTask()
{
    m_watcher.waitForFinished();
    cleanUp();
}

QString ExportTask::stagingRoot(const QString& dataRoot)
{
    return QDir(dataRoot).absoluteFilePath(".export-staging");
}

void ExportTask::executeTask()
{
    if (OfflineMode::globallyOffline()) {
        fail(Messages::exportNeedsInternet(), "the launcher is in offline mode");
        return;
    }
    setAbortable(true);
    m_dataRoot = QDir::currentPath();
    m_staging = QDir(stagingRoot(m_dataRoot)).absoluteFilePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!QDir().mkpath(m_staging)) {
        fail(Messages::couldNotWriteBundle(), QString("can't create %1").arg(m_staging));
        return;
    }
    m_manifest.formatVersion = SupportedFormatVersion;
    m_manifest.kind = m_request.kind;
    m_manifest.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_manifest.createdBy = BuildConfig.LAUNCHER_DISPLAYNAME + ' ' + BuildConfig.printableVersionString();

    bool ok = true;
    if (m_request.kind == Kind::Instance) {
        ok = addInstanceJob();
    } else {
        QStringList labels;
        for (int i = 0; ok && i < m_request.sets.size(); ++i) {
            ok = addSetJob(i, m_request.sets.at(i));
            labels << m_jobs.back().label;
        }
        m_manifest.name = labels.join(", ");
    }
    if (!ok)
        return;  // fail() was called
    if (m_jobs.empty()) {
        fail(Messages::couldNotWriteBundle(), "nothing to export");
        return;
    }
    startJob();
}

std::unique_ptr<MinecraftInstance> ExportTask::makeInstance(const QString& dir)
{
    auto inst = std::make_unique<MinecraftInstance>(APPLICATION->settings(),
                                                    std::make_unique<INISettingsObject>(FS::PathCombine(dir, "instance.cfg")), dir);
    // Windows x64 natives whatever PC this runs on. The architecture settings pass through to the global settings
    // unless the instance overrides the Java location, so turn that on first (this instance is thrown away).
    inst->settings()->set("OverrideJavaLocation", true);
    inst->settings()->set("JavaArchitecture", m_context.javaArchitecture);
    inst->settings()->set("JavaRealArchitecture", m_context.javaRealArchitecture);
    inst->updateRuntimeContext();
    return inst;
}

bool ExportTask::addSetJob(int index, const ExportSet& set)
{
    const QString dir = QDir(m_staging).absoluteFilePath(QString("set%1").arg(index));
    QDir().mkpath(dir);
    auto inst = makeInstance(dir);
    QString label = tr("Minecraft %1").arg(set.minecraft);
    {
        SettingsObject::Lock lock(inst->settings());
        auto* components = inst->getPackProfile();
        components->buildingFromScratch();
        components->setComponentVersion("net.minecraft", set.minecraft, true);
        if (!set.loaderUid.isEmpty()) {
            components->setComponentVersion(set.loaderUid, set.loaderVersion);
            QString loaderName = loaderDisplayName(set.loaderUid);
            if (loaderName.isEmpty())
                loaderName = set.loaderUid;
            label += QString(" + %1 %2").arg(loaderName, set.loaderVersion);
        }
        inst->setName(label);
        components->saveNow();
    }
    auto addComponent = [this](const ComponentEntry& entry) {
        for (const ComponentEntry& c : m_manifest.components)
            if (c.uid == entry.uid && c.version == entry.version)
                return;
        m_manifest.components.append(entry);
    };
    addComponent({ "net.minecraft", set.minecraft, "Minecraft" });
    if (!set.loaderUid.isEmpty()) {
        const QString name = loaderDisplayName(set.loaderUid);
        addComponent({ set.loaderUid, set.loaderVersion, name.isEmpty() ? set.loaderUid : name });
    }
    m_jobs.push_back({ label, std::move(inst) });
    return true;
}

bool ExportTask::addInstanceJob()
{
    BaseInstance* source = APPLICATION->instances()->getInstanceById(m_request.instanceId);
    auto* mc = dynamic_cast<MinecraftInstance*>(source);
    if (!mc) {
        fail(Messages::instanceNotFound(), QString("instance %1 not found").arg(m_request.instanceId));
        return false;
    }
    // Copy only what resolution needs; the real instance folder is read later and never changed.
    const QString dir = QDir(m_staging).absoluteFilePath("instance");
    QDir().mkpath(dir);
    for (const QString& name : { "instance.cfg", "mmc-pack.json" })
        QFile::copy(FS::PathCombine(mc->instanceRoot(), name), FS::PathCombine(dir, name));
    for (const QString& name : { "patches", "libraries", "jarmods" }) {
        const QString from = FS::PathCombine(mc->instanceRoot(), name);
        if (QFileInfo(from).isDir())
            FS::copy(from, FS::PathCombine(dir, name))();
    }
    auto inst = makeInstance(dir);
    m_manifest.name = mc->name();
    m_manifest.instance = InstanceEntry{ mc->name(), "instance", APPLICATION->instances()->getInstanceGroup(mc->id()) };
    m_jobs.push_back({ mc->name(), std::move(inst) });
    return true;
}

void ExportTask::runTask(Task::Ptr task, const QString& item, std::function<void()> next)
{
    m_current = task;
    m_keepAlive.append(task);
    connect(task.get(), &Task::succeeded, this, [this, next] {
        if (m_cancelled)
            finishCancelled();
        else
            next();
    });
    connect(task.get(), &Task::failed, this, [this, item](const QString& reason) { fail(Messages::downloadFailed(item), reason); });
    connect(task.get(), &Task::aborted, this, &ExportTask::finishCancelled);
    connect(task.get(), &Task::status, this, &ExportTask::setDetails);
    connect(task.get(), &Task::progress, this, &ExportTask::setProgress);
    task->start();
}

void ExportTask::startJob()
{
    Job& job = m_jobs.at(m_jobIndex);
    setStatus(tr("Preparing %1 (%2 of %3)...").arg(job.label).arg(m_jobIndex + 1).arg(m_jobs.size()));
    MinecraftInstance* inst = job.instance.get();
    runTask(makeShared<MinecraftLoadAndCheck>(inst, Net::Mode::Online), job.label, [this, inst] {
        runTask(makeShared<LibrariesTask>(inst), m_jobs.at(m_jobIndex).label, [this, inst] {
            runTask(makeShared<AssetUpdateTask>(inst), tr("the game sounds and textures"), [this, inst] {
                auto profile = inst->getPackProfile()->getProfile();
                const QString javaName = profile->getCompatibleJavaName();
                const QList<int> majors = profile->getCompatibleJavaMajors();
                auto java = makeShared<ExportJavaTask>(javaName, majors, QDir(m_dataRoot).absoluteFilePath(APPLICATION->javaPath()));
                runTask(java, tr("Java for %1").arg(m_jobs.at(m_jobIndex).label),
                        [this, inst, javaName, majors] { afterJava(inst, javaName, majors); });
            });
        });
    });
}

static QString firstFile(const LibraryPtr& lib, const RuntimeContext& ctx, const QString& overridePath)
{
    if (!lib)
        return {};
    QStringList jars, natives, natives32, natives64;
    lib->getApplicableFiles(ctx, jars, natives, natives32, natives64, overridePath);
    return jars.value(0);
}

void ExportTask::afterJava(MinecraftInstance* inst, const QString& javaFolderName, const QList<int>& majors)
{
    if (!addJavaFolder(javaFolderName, majors.value(0)))
        return;
    auto profile = inst->getPackProfile()->getProfile();
    if (profile->getMainClass() != s_forgeWrapperMain) {
        collectJob(inst, {});
        return;
    }
    const QString local = inst->getLocalLibraryPath();
    QString wrapper, installer;
    for (const LibraryPtr& lib : profile->getLibraries())
        if (lib->artifactPrefix() == "io.github.zekerzhayard:ForgeWrapper")
            wrapper = firstFile(lib, m_context, local);
    for (const LibraryPtr& lib : profile->getMavenFiles())
        if (lib->rawName().classifier() == "installer")
            installer = firstFile(lib, m_context, local);
    const QString minecraft = firstFile(profile->getMainJar(), m_context, local);
    if (wrapper.isEmpty() || installer.isEmpty() || minecraft.isEmpty()) {
        fail(Messages::downloadFailed(m_jobs.at(m_jobIndex).label),
             QString("Forge files not found: wrapper=%1 installer=%2 minecraft=%3").arg(wrapper, installer, minecraft));
        return;
    }
    const QString javaExe = FS::PathCombine(QDir(m_dataRoot).absoluteFilePath(APPLICATION->javaPath()), javaFolderName, "bin", "java.exe");
    auto prep = makeShared<ForgePrepTask>(javaExe, APPLICATION->getJarPath("pinecone-forge-prep.jar"), wrapper, installer,
                                          QDir(m_dataRoot).absoluteFilePath("libraries"), minecraft);
    runTask(prep, m_jobs.at(m_jobIndex).label, [this, inst, prep] {
        qDebug() << "Forge processors created" << prep->createdFiles().size() << "files";
        collectJob(inst, prep->createdFiles());
    });
}

void ExportTask::collectJob(MinecraftInstance* inst, const QStringList& forgeOutputs)
{
    setStatus(tr("Collecting files for %1...").arg(m_jobs.at(m_jobIndex).label));
    auto* packProfile = inst->getPackProfile();
    auto profile = packProfile->getProfile();
    const QString librariesRoot = QDir(m_dataRoot).absoluteFilePath("libraries");

    QList<LibraryPtr> libraries = profile->getLibraries();
    libraries += profile->getNativeLibraries();
    libraries += profile->getMavenFiles();
    for (const auto& agent : profile->getAgents())
        libraries.append(agent.library);
    libraries.append(profile->getMainJar());
    addLibraries(m_files, libraries, m_context, inst->getLocalLibraryPath(), librariesRoot);
    for (const QString& file : forgeOutputs)
        addFileUnder(m_files, file, librariesRoot, "libraries");

    const QString metaRoot = QDir(m_dataRoot).absoluteFilePath("meta");
    for (int i = 0; i < packProfile->rowCount(QModelIndex()); ++i) {
        auto component = packProfile->getComponent(i);
        if (!component || component->isCustom())
            continue;
        addMeta(m_files, metaRoot, component->getID(), component->getVersion());
        if (m_request.kind == Kind::Instance && (component->getID() == "net.minecraft" || !loaderDisplayName(component->getID()).isEmpty()))
            m_manifest.components.append({ component->getID(), component->getVersion(), component->getName() });
    }

    if (auto assets = profile->getMinecraftAssets()) {
        if (!addAssets(m_files, QDir(m_dataRoot).absoluteFilePath("assets"), assets->id)) {
            fail(Messages::downloadFailed(tr("the game sounds and textures")), QString("asset index %1 is missing").arg(assets->id));
            return;
        }
    }
    addElyPatch(inst);
}

// The Ely.by authlib patch (or authlib-injector) that ElyPatchTask adds at launch for Ely.by accounts. The normal
// libraries are already collected, so this only adds files. Failing here is not fatal.
void ExportTask::addElyPatch(MinecraftInstance* inst)
{
    auto ely = makeShared<ElyPatchTask>(inst, m_context, Net::Mode::Online);
    m_current = ely;
    m_keepAlive.append(ely);
    auto done = [this, inst](bool ok) {
        if (m_cancelled) {
            finishCancelled();
            return;
        }
        if (!ok) {
            qWarning() << "Export: the Ely.by patch couldn't be fetched; the bundle won't include it";
            nextJob();
            return;
        }
        runTask(makeShared<LibrariesTask>(inst), tr("the Ely.by patch"), [this, inst] {
            auto profile = inst->getPackProfile()->getProfile();
            addLibraries(m_files, profile->getLibraries(), m_context, inst->getLocalLibraryPath(),
                         QDir(m_dataRoot).absoluteFilePath("libraries"));
            const QString metaRoot = QDir(m_dataRoot).absoluteFilePath("meta");
            for (const QString& uid : { "by.ely.authlib", "moe.yushi.authlibinjector" }) {
                auto list = APPLICATION->metadataIndex()->get(uid);
                for (int i = 0; i < list->count(); ++i) {
                    auto version = list->concreteAt(i);
                    if (version->isLoaded())
                        addMeta(m_files, metaRoot, uid, version->version());
                }
            }
            nextJob();
        });
    };
    connect(ely.get(), &Task::succeeded, this, [done] { done(true); });
    connect(ely.get(), &Task::failed, this, [done] { done(false); });
    connect(ely.get(), &Task::aborted, this, &ExportTask::finishCancelled);
    ely->start();
}

void ExportTask::nextJob()
{
    ++m_jobIndex;
    if (m_jobIndex < m_jobs.size()) {
        startJob();
        return;
    }
    if (m_request.kind == Kind::Instance) {
        auto* mc = dynamic_cast<MinecraftInstance*>(APPLICATION->instances()->getInstanceById(m_request.instanceId));
        const QString root = mc->instanceRoot();
        const bool worlds = m_request.includeWorlds;
        addTree(m_files, root, "instance",
                [worlds](const QString& rel) { return rel != "instance.cfg" && shouldExportInstanceFile(rel, worlds); });
        QFile cfg(FS::PathCombine(root, "instance.cfg"));
        if (!cfg.open(QIODevice::ReadOnly)) {
            fail(Messages::couldNotWriteBundle(), QString("can't read %1").arg(cfg.fileName()));
            return;
        }
        m_files.addData("instance/instance.cfg", sanitizeInstanceCfg(cfg.readAll()));
    }
    for (const QString& folder : m_request.extraJavaFolders)
        if (!addJavaFolder(folder, 0))
            return;
    startWriting();
}

bool ExportTask::addJavaFolder(const QString& folderName, int fallbackMajor)
{
    if (m_javaFolders.contains(folderName))
        return true;
    const QString dir = QDir(QDir(m_dataRoot).absoluteFilePath(APPLICATION->javaPath())).absoluteFilePath(folderName);
    if (!addTree(m_files, dir, "java/" + folderName)) {
        fail(Messages::downloadFailed(tr("Java")), QString("%1 doesn't exist").arg(dir));
        return false;
    }
    m_javaFolders.append(folderName);
    QFile release(FS::PathCombine(dir, "release"));
    int major = release.open(QIODevice::ReadOnly) ? javaMajorFromRelease(release.readAll()) : 0;
    if (major == 0)
        major = fallbackMajor;
    m_manifest.java.append({ major > 0 ? tr("Java %1").arg(major) : folderName, major, folderName });
    return true;
}

void ExportTask::startWriting()
{
    if (!m_files.missing().isEmpty()) {
        fail(Messages::downloadFailed(tr("%n file(s)", "", m_files.missing().size())),
             "missing files:\n" + m_files.missing().mid(0, 30).join('\n'));
        return;
    }
    if (!m_files.conflicts().isEmpty()) {
        fail(Messages::couldNotWriteBundle(), "two files want the same place in the bundle:\n" + m_files.conflicts().mid(0, 30).join('\n'));
        return;
    }
    const qint64 needed = m_files.totalSize() + 64 * 1024 * 1024;
    const QStorageInfo storage(QFileInfo(m_request.outputPath).absolutePath());
    if (storage.isValid() && storage.bytesAvailable() < needed) {
        fail(Messages::notEnoughSpace(needed, storage.bytesAvailable()), storage.rootPath());
        return;
    }
    setStatus(tr("Writing the bundle..."));
    m_current.reset();
    m_future = QtConcurrent::run(QThreadPool::globalInstance(), [this] {
        return writeBundle(
            m_request.outputPath, m_manifest, m_files,
            [this](const QString& step, qint64 done, qint64 total) {
                setStatus(step);
                setProgress(done, total);
            },
            m_cancelled);
    });
    m_watcher.setFuture(m_future);
}

void ExportTask::finishWriting()
{
    const WriteResult result = m_future.result();
    if (result.cancelled) {
        finishCancelled();
    } else if (!result.ok) {
        fail(result.message, result.details);
    } else {
        m_bundleSize = result.zipSize;
        m_done = true;
        cleanUp();
        emitSucceeded();
    }
}

bool ExportTask::abort()
{
    m_cancelled = true;
    if (m_current && m_current->isRunning())
        m_current->abort();
    else if (!m_future.isRunning())
        finishCancelled();
    return true;
}

void ExportTask::fail(const QString& message, const QString& details)
{
    if (m_done)
        return;
    m_done = true;
    m_errorDetails = details;
    qWarning() << "Export failed:" << message << details;
    // fail() usually runs inside a step's signal, while that step may still use the throwaway instance: clean up later.
    QTimer::singleShot(0, this, [this] { cleanUp(); });
    emitFailed(message);
}

void ExportTask::finishCancelled()
{
    if (m_done)
        return;
    m_done = true;
    QTimer::singleShot(0, this, [this] { cleanUp(); });
    emitAborted();
}

void ExportTask::cleanUp()
{
    m_current.reset();
    m_keepAlive.clear();
    m_jobs.clear();  // the throwaway instances must be gone before their folders
    if (!m_staging.isEmpty())
        FS::deletePath(m_staging);
    m_staging.clear();
}

}  // namespace OfflineBundle
