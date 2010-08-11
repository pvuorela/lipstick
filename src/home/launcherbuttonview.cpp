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

#include <MProgressIndicator>
#include "launcherbuttonview.h"
#include "launcherbutton.h"

LauncherButtonView::LauncherButtonView(LauncherButton *controller) :
    MButtonIconView(controller),
    controller(controller),
    progressIndicator(new MProgressIndicator(controller, MProgressIndicator::spinnerType))
{
    progressIndicator->setContentsMargins(0,0,0,0);
    progressIndicator->setRange(0, 100);
    progressIndicator->hide();

    // When the progress indicator timer times out the progress indicator should be hidden
    launchProgressTimeoutTimer.setSingleShot(true);
    connect(&launchProgressTimeoutTimer, SIGNAL(timeout()), controller, SLOT(stopLaunchProgress()));
}

LauncherButtonView::~LauncherButtonView()
{
}

void LauncherButtonView::applyStyle()
{
    MButtonIconView::applyStyle();

    // set launch progress maximum duration from style
    launchProgressTimeoutTimer.setInterval(style()->launchProgressIndicatorTimeout());

    if (controller->objectName() == "LauncherButton") {
        progressIndicator->setObjectName("LauncherButtonProgressIndicator");
    } else {
        progressIndicator->setObjectName("QuickLaunchBarButtonProgressIndicator");
    }

    // Set position and size for progress indicator
    int hMargin = style()->paddingLeft() + style()->paddingRight() + style()->marginLeft() + style()->marginRight();
    int vMargin = style()->paddingTop() + style()->paddingBottom() + style()->marginTop() + style()->marginBottom();

    int progressIndicatorHOffset = (style()->preferredSize().width() / 2) - (style()->progressIndicatorIconSize().width() / 2);
    int progressIndicatorVOffset = (style()->iconSize().height() - style()->progressIndicatorIconSize().height())/2;
    int progressIndicatorLeftPosition =  progressIndicatorHOffset + hMargin / 2;
    int progressIndicatorTopPosition = progressIndicatorVOffset + vMargin / 2;

    progressIndicator->setPreferredSize(style()->progressIndicatorIconSize());
    progressIndicator->setPos(QPointF(progressIndicatorLeftPosition, progressIndicatorTopPosition));
}

void LauncherButtonView::setupModel()
{
    MButtonIconView::setupModel();

    resetProgressIndicator();
}

void LauncherButtonView::updateData(const QList<const char *>& modifications)
{
    MButtonIconView::updateData(modifications);

    const char *member;
    foreach(member, modifications) {
        if (member == LauncherButtonModel::ButtonState) {
            resetProgressIndicator();

            if (model()->buttonState() == LauncherButtonModel::Launching) {
                launchProgressTimeoutTimer.start();
            } else {
                // stop launch timer in case we were launching
                if (launchProgressTimeoutTimer.isActive()) {
                    launchProgressTimeoutTimer.stop();
                }
            }
        } else if (member == LauncherButtonModel::OperationProgress) {
            if (model()->buttonState() == LauncherButtonModel::Downloading) {
                progressIndicator->setValue(model()->operationProgress());
            }
        }
    }
}

void LauncherButtonView::resetProgressIndicator()
{
    switch(model()->buttonState()) {
        case LauncherButtonModel::Installing:
        case LauncherButtonModel::Launching:
        {
            progressIndicator->reset();
            progressIndicator->setUnknownDuration(true);
            progressIndicator->show();
        }
        break;
        case LauncherButtonModel::Downloading:
        {
            progressIndicator->reset();
            progressIndicator->show();
        }
        break;
        case LauncherButtonModel::Installed:
        case LauncherButtonModel::Broken:
        default:
        {
            progressIndicator->hide();
        }
        break;
    }
}

M_REGISTER_VIEW_NEW(LauncherButtonView, LauncherButton)
