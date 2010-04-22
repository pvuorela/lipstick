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
#ifndef _UT_LAUNCHER_
#define _UT_LAUNCHER_

#include <QtTest/QtTest>
#include <QObject>
#include <QVector>

class Launcher;
class MApplication;
class MWidget;

class Ut_Launcher : public QObject
{
    Q_OBJECT

public:
    static QFileInfoList desktopFileInfoList;
    static QFileInfoList directoryFileInfoList;
    static QString       applicationStarted;
    static bool          mApplicationIfProxyLaunchCalled;

private:
    //TestLauncher *launcher;
    Launcher *launcher;
    MApplication *app;

    void writeDesktopFile(QString fileName, QString type, QString name, QString iconName, QString exec);
    int buttonsCount();

signals:
    void directoryLaunched(const QString &directory, const QString &title = QString(), const QString &iconId = QString());
    void directoryChanged(const QString path);
    void applicationLaunched(const QString &service);
    void mApplicationLaunched(const QString &service);
    void buttonClicked();
    void timerTimedOut();
private slots:
    // Executed once before every test case
    void init();
    // Executed once after every test case
    void cleanup();
    // Executed once before first test case
    void initTestCase();
    // Executed once after last test case
    void cleanupTestCase();

    // Test that the launcher initialization creates items for all desktop entries
    void testInitialization();
    // Test that the timer for button storing is started and
    // that the thread for button storing is also started
    void testButtonStoreTimerAndThread();
    // Test that the launcher includes an entry that is supposed to be shown in DUI
    void testOnlyShowInDUI();
    // Test that the launcher doesn't include an entry that isn't supposed to be shown in DUI
    void testOnlyShowInNotDUI();
    // Test that the launcher doesn't include an entry that is not supposed to be shown in DUI
    void testNotShowInDUI();
    // Test that the launcher includes an entry that is not supposed to be shown in some other environment than DUI
    void testNotShowInNotDUI();
    // Test that adding a new desktop entry to root adds a new widget
    void testDesktopEntryAdd();
    // Test that invalid files are not added
    void testInvalidFiles();
    // Test that removing a new desktop entry removes the widget
    void testDesktopEntryRemove();
    // Test that launching an Application is attempted
    void testApplicationLaunched();
    // Test that launching a MApplication is attempted
    void testMApplicationLaunched();
    // Test that launcher buttons are paged to multiple pages
    void testPaging();
    // Test that empty page is removed from launcher
    void testEmptyPage();
    // Test that with multiple pages buttons are added correctly and remain correct when buttons removed
    void testAddingAndRemovingButtonsWithMultiplePages();
};
#endif //_UT_LAUNCHER_
