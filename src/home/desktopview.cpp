/***************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (directui@nokia.com)
**
** This file is part of mhome.
**
** If you have questions regarding the use of this file, please contact
** Nokia at directui@nokia.com.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/
#ifdef BENCHMARKS_ON
#include <QTextStream>
#include <QFile>
#include <QTimer>
#include <QTime>
#include <QFileSystemWatcher>
#endif

#include <QDir>

#include "launcher.h"
#include "launcherdatastore.h"
#include "desktopview.h"
#include "desktop.h"
#include "mainwindow.h"
#include "homewindowmonitor.h"
#include "switcher.h"
#include "quicklaunchbar.h"
#include "mdesktopbackgroundextensioninterface.h"
#include "mfiledatastore.h"
#include "applicationpackagemonitor.h"

#include <MViewCreator>
#include <MDeviceProfile>
#include <MSceneManager>
#include <MSceneWindow>
#include <MPannableViewport>
#include <MApplication>
#include <MOverlay>
#include <MApplicationExtensionArea>
#include <QGraphicsLinearLayout>

#ifdef BENCHMARKS_ON

// These should really be private variables if one has more than one
// instance of Desktop
static bool benchmarking = false;
static QTime lastUpdate;
static int frameCount = 0;
static int fps = 0;
static QFile* fpsFile;
static QTextStream* fpsStream;

const int MillisecsInSec = 1000;
const int FpsRefreshInterval = 1000;

void DesktopView::writeFps()
{
    if (!benchmarking)
        return;

    QString fpsString = QString::number(fps);
    QDateTime now = QDateTime::currentDateTime();
    QString nowString = now.toString(Qt::ISODate);

    *fpsStream << fpsString << " " << nowString << endl;
    fpsStream->flush();
}

void DesktopView::startBenchmarking()
{
    QDir dir;
    if(!dir.exists("/tmp/duihome_benchmarks")) {
        dir.mkdir("/tmp/duihome_benchmarks");
    }

    fpsFile = new QFile("/tmp/duihome_benchmarks/benchmark_results.txt");
    fpsFile->open(QIODevice::WriteOnly | QIODevice::Append);
    fpsStream = new QTextStream(fpsFile);

    frameCount = 0;
    fps = 0;
    lastUpdate = QTime::currentTime();
    benchmarking = true;

    update();
}

void DesktopView::stopBenchmarking()
{
    benchmarking = false;

    delete fpsStream;
    delete fpsFile;
}
#endif

DesktopView::DesktopView(Desktop *desktop) :
    MWidgetView(desktop),
    windowMonitor(new HomeWindowMonitor),
    switcher(new Switcher(windowMonitor)),
    switcherWindow(new MSceneWindow),
    switcherHasContent(false),
    launcherDataStore(NULL),
    launcher(NULL),
    launcherWindow(new MSceneWindow),
    launcherVisible(false),
    quickLaunchBar(NULL),
    quickLaunchBarWindow(new MOverlay),
    backgroundExtensionArea(new MApplicationExtensionArea("com.meego.core.MDesktopBackgroundExtensionInterface/1.0"))
{
    windowMonitor->registerWindowId(MainWindow::instance()->winId());

    // Add the switcher into a scene window
    switcher->setObjectName("OverviewSwitcher");
    QGraphicsLinearLayout *windowLayout = new QGraphicsLinearLayout();
    windowLayout->setContentsMargins(0, 0, 0, 0);
    switcherWindow->setLayout(windowLayout);
    switcherWindow->setObjectName("SwitcherWindow");
    windowLayout->addItem(switcher);
    MainWindow::instance()->sceneManager()->appearSceneWindowNow(switcherWindow);

    // Create the launcher data store
    launcherDataStore = createLauncherDataStore();

    // Create application package monitor
    packageMonitor = new ApplicationPackageMonitor();

    // Create a quick launch bar and put it in a scene window
    quickLaunchBar = new QuickLaunchBar(launcherDataStore);
    connect(quickLaunchBar, SIGNAL(toggleLauncherButtonClicked()), this, SLOT(toggleLauncher()));
    windowLayout = new QGraphicsLinearLayout();
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->addItem(quickLaunchBar);
    quickLaunchBarWindow->setLayout(windowLayout);
    quickLaunchBarWindow->setObjectName("QuickLaunchBarOverlay");
    MainWindow::instance()->sceneManager()->appearSceneWindowNow(quickLaunchBarWindow);

    // Add the launcher into a scene window
    launcher = new Launcher(launcherDataStore, packageMonitor);
    connect(qApp, SIGNAL(focusToLauncherAppRequested(const QString &)), this, SLOT(showLauncherAndPanToPage(const QString &)));
    connect(switcher, SIGNAL(windowStackingOrderChanged(const QList<WindowInfo> &)), this, SLOT(updateLauncherVisiblity(const QList<WindowInfo> &)));
    connect(switcher, SIGNAL(windowListUpdated(const QList<WindowInfo> &)), this, SLOT(setSwitcherHasContent(const QList<WindowInfo> &)));
    windowLayout = new QGraphicsLinearLayout();
    windowLayout->setContentsMargins(0, 0, 0, 0);
    launcherWindow->setLayout(windowLayout);
    launcherWindow->setObjectName("LauncherWindow");
    windowLayout->addItem(launcher);

    // Register the launcher window into the scene manager to make sure launcher page styling works in both orientations
    MainWindow::instance()->sceneManager()->appearSceneWindowNow(launcherWindow);
    MainWindow::instance()->sceneManager()->disappearSceneWindowNow(launcherWindow);

#ifdef BENCHMARKS_ON
    connect(MApplication::instance(), SIGNAL(startBenchmarking()), this, SLOT(startBenchmarking()));
    connect(MApplication::instance(), SIGNAL(stopBenchmarking()), this, SLOT(stopBenchmarking()));
#endif

    // Connect the desktop background extension signals
    connect(backgroundExtensionArea, SIGNAL(extensionInstantiated(MApplicationExtensionInterface*)),
            this, SLOT(addExtension(MApplicationExtensionInterface*)));
    connect(backgroundExtensionArea, SIGNAL(extensionRemoved(MApplicationExtensionInterface*)),
            this, SLOT(removeExtension(MApplicationExtensionInterface*)));
    backgroundExtensionArea->setInProcessFilter(QRegExp("/duihome-plaindesktopbackgroundextension.desktop$"));
    backgroundExtensionArea->setOutOfProcessFilter(QRegExp("$^"));
    backgroundExtensionArea->init();

    setSceneWindowOrder();
}

DesktopView::~DesktopView()
{
    delete switcherWindow;
    delete launcherWindow;
    delete quickLaunchBarWindow;
    delete backgroundExtensionArea;
    delete launcherDataStore;
    delete windowMonitor;
    delete packageMonitor;
}

#ifdef BENCHMARKS_ON
void DesktopView::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    MWidgetView::paint(painter, option, widget);

    if (benchmarking) {
        QTime now = QTime::currentTime();
        ++frameCount;

        if (lastUpdate.msecsTo(now) > FpsRefreshInterval) {
            fps = (MillisecsInSec * frameCount) / (lastUpdate.msecsTo(now));
            frameCount = 0;
            lastUpdate = now;

            writeFps();
        }

        update();
    }
}
#endif

void DesktopView::drawBackground(QPainter *painter, const QStyleOptionGraphicsItem *) const
{
    foreach (MDesktopBackgroundExtensionInterface *backgroundExtension, backgroundExtensions) {
        backgroundExtension->drawBackground(painter, boundingRect());
    }
}

void DesktopView::showLauncherAndPanToPage(const QString &desktopFileEntry)
{
    if(launcher->panToPage(desktopFileEntry) >= 0 || desktopFileEntry.isEmpty()) {
        showLauncher();
        MainWindow::instance()->activateWindow();
        MainWindow::instance()->raise();
    }
}

void DesktopView::updateLauncherVisiblity(const QList<WindowInfo> &windowList)
{
    if (launcherWindow->isVisible() && !windowList.isEmpty()) {
        const QList<Atom>& windowTypes = windowList.last().types();
        if (!windowTypes.contains(WindowInfo::NotificationAtom) &&
            !windowTypes.contains(WindowInfo::DesktopAtom) &&
            !windowTypes.contains(WindowInfo::DialogAtom) &&
            !windowTypes.contains(WindowInfo::MenuAtom)) {
            hideLauncher();
        }
    }
}

void DesktopView::toggleLauncher()
{
    if (launcherWindow->isVisible()) {
        hideLauncher();
    } else {
        launcher->setFirstPage();
        showLauncher();
    }
}

void DesktopView::showLauncher()
{
    MainWindow::instance()->sceneManager()->appearSceneWindow(launcherWindow);
    MainWindow::instance()->sceneManager()->disappearSceneWindow(switcherWindow);
    setSceneWindowOrder();

    launcherVisible = true;
    setDefocused();
}

void DesktopView::hideLauncher()
{
    MainWindow::instance()->sceneManager()->disappearSceneWindow(launcherWindow);
    MainWindow::instance()->sceneManager()->appearSceneWindow(switcherWindow);
    setSceneWindowOrder();

    launcherVisible = false;
    setDefocused();
}

void DesktopView::setSceneWindowOrder()
{
    // Keep the switcher and launcher windows behind the quick launch bar window
    launcherWindow->setZValue(quickLaunchBarWindow->zValue() - 1);
    switcherWindow->setZValue(quickLaunchBarWindow->zValue() - 2);
}

void DesktopView::setSwitcherHasContent(const QList<WindowInfo> &windowList)
{
    switcherHasContent = !windowList.isEmpty();
    setDefocused();
}

void DesktopView::setDefocused()
{
    bool defocused = switcherHasContent || launcherVisible;
    foreach (MDesktopBackgroundExtensionInterface *backgroundExtension, backgroundExtensions) {
        backgroundExtension->setDefocused(defocused);
    }
}

void DesktopView::update()
{
    MWidgetView::update();
}

M::OrientationAngle DesktopView::orientationAngle()
{
    return MainWindow::instance()->sceneManager()->orientationAngle();
}

void DesktopView::addExtension(MApplicationExtensionInterface *extension)
{
    MDesktopBackgroundExtensionInterface *backgroundExtension = static_cast<MDesktopBackgroundExtensionInterface *>(extension);
    backgroundExtension->setDesktopInterface(*this);
    backgroundExtensions.append(backgroundExtension);
}

void DesktopView::removeExtension(MApplicationExtensionInterface *extension)
{
    MDesktopBackgroundExtensionInterface *backgroundExtension = static_cast<MDesktopBackgroundExtensionInterface *>(extension);
    backgroundExtensions.removeOne(backgroundExtension);
}

LauncherDataStore *DesktopView::createLauncherDataStore()
{
    if (!QDir::root().exists(QDir::homePath() + "/.config/duihome")) {
        QDir::root().mkpath(QDir::homePath() + "/.config/duihome");
    }

    QString dataStoreFileName = QDir::homePath() + "/.config/duihome/launcherbuttons.data";

    if (!QFile::exists(dataStoreFileName)) {
        QString defaultDataStoreFileName = M_XDG_DIR "/duihome/launcherbuttons.data";
        // Copy the default datastore only if it exists
        if (QFile::exists(defaultDataStoreFileName)) {
            QFile::copy(defaultDataStoreFileName, dataStoreFileName);
        }
    }

    return new LauncherDataStore(new MFileDataStore(dataStoreFileName));
}

M_REGISTER_VIEW_NEW(DesktopView, Desktop)
