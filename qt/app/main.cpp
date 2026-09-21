#include "AppController.h"
#include "SelfTest.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFont>
#include <QGuiApplication>
#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

int main(int argc, char *argv[])
{
    // Must run before the QGuiApplication is constructed.
    QtWebEngineQuick::initialize();

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("广轻活动汇总"));
    app.setOrganizationName(QStringLiteral("GdipuActivityHelper"));
    app.setApplicationVersion(QStringLiteral("2.1.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Gdipu/assets/app-logo.png")));

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({QStringLiteral("self-test"), QStringLiteral("Run the end-to-end self-test against the mock site."), QStringLiteral("dir")});
    parser.addOption({QStringLiteral("base-url"), QStringLiteral("Portal base URL (self-test uses http://127.0.0.1:8765/fixture)."), QStringLiteral("url")});
    parser.addOption({QStringLiteral("schedule-fixture"), QStringLiteral("Extra academic timetable page for the self-test."), QStringLiteral("url")});
    parser.addOption({QStringLiteral("demo"), QStringLiteral("Populate the UI from an activities.json file (development)."), QStringLiteral("file")});
    parser.addOption({QStringLiteral("screenshot"), QStringLiteral("Save one PNG per page into <dir> and exit."), QStringLiteral("dir")});
    parser.addOption({QStringLiteral("theme"), QStringLiteral("light | dark | system for this run."), QStringLiteral("mode")});
    parser.process(app);

    AppOptions options;
    options.testing = parser.isSet(QStringLiteral("self-test"));
    options.testDir = QDir::fromNativeSeparators(parser.value(QStringLiteral("self-test")));
    options.baseUrl = parser.value(QStringLiteral("base-url"));
    if (options.testing && options.baseUrl.isEmpty())
        options.baseUrl = QStringLiteral("http://127.0.0.1:8765/fixture");
    options.scheduleFixture = parser.value(QStringLiteral("schedule-fixture"));
    options.demoActivities = parser.value(QStringLiteral("demo"));
    options.screenshotDir = QDir::fromNativeSeparators(parser.value(QStringLiteral("screenshot")));
    options.theme = parser.value(QStringLiteral("theme"));

    // The icon font on Windows 11 is "Segoe Fluent Icons"; Windows 10 only has MDL2 Assets.
    QFont::insertSubstitutions(QStringLiteral("Segoe Fluent Icons"), {QStringLiteral("Segoe MDL2 Assets")});
    QFont font(QStringLiteral("Microsoft YaHei UI"));
    font.setPixelSize(14);
    app.setFont(font);
    QQuickStyle::setStyle(QStringLiteral("Basic"));   // fully restyled in QML (see Theme.qml)

    AppController controller(options);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(2); }, Qt::QueuedConnection);
    engine.loadFromModule("Gdipu", "Main");
    if (engine.rootObjects().isEmpty())
        return 2;

    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst());
    if (options.testing) {
        QTimer::singleShot(800, &app, [&] { runSelfTest(controller, window); });
    } else if (!options.screenshotDir.isEmpty()) {
        QTimer::singleShot(800, &app, [&] { runScreenshots(controller, window); });
    } else if (!options.demoActivities.isEmpty()) {
        controller.loadDemo(options.demoActivities);
    }
    return app.exec();
}
