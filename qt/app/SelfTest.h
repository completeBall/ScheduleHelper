#pragma once

#include "AppController.h"

class QQuickWindow;

// --self-test <dir>: drives the real UI + embedded browser against the mock school
// site (node server.mjs) and checks the same things the C# build's self-test did.
// Writes <dir>/result.json and <dir>/self-test.log; returns via QCoreApplication::exit.
void runSelfTest(AppController &app, QQuickWindow *window);

// --screenshot <dir>: saves activities.png / schedule.png / browser.png and exits.
void runScreenshots(AppController &app, QQuickWindow *window);
