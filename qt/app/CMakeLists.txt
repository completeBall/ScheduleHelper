find_package(Qt6 6.8 REQUIRED COMPONENTS Quick QuickControls2 QuickDialogs2 WebEngineQuick Widgets Network)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(GdipuActivityHelper WIN32
    app.rc
    main.cpp
    AppController.cpp
    UpdateManager.cpp
    ActivityModel.cpp
    ScheduleModel.cpp
    ScheduleImporter.cpp
    Scraper.cpp
    SelfTest.cpp
    WebBridge.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(GdipuActivityHelper
    URI Gdipu
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/AppButton.qml
        qml/components/Card.qml
        qml/components/FluentIcon.qml
        qml/components/MetricCard.qml
        qml/components/StatusBadge.qml
        qml/components/SearchField.qml
        qml/components/FilterCombo.qml
        qml/components/NavButton.qml
        qml/components/Toast.qml
        qml/components/ProgressBanner.qml
        qml/components/FormField.qml
        qml/components/CourseCard.qml
        qml/components/WebPane.qml
        qml/components/EmptyState.qml
        qml/pages/ActivitiesPage.qml
        qml/pages/SchedulePage.qml
        qml/pages/BrowserPage.qml
        qml/pages/SettingsDialog.qml
        qml/pages/DateDialog.qml
        qml/pages/CourseDialog.qml
        qml/pages/CourseDetailDialog.qml
        qml/pages/SlotDialog.qml
        qml/components/BookmarkButton.qml
    RESOURCES
        assets/app-logo.png
)

qt_add_resources(GdipuActivityHelper scripts
    PREFIX /gdipu
    BASE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES js/extract.js js/install-update.ps1
)

target_include_directories(GdipuActivityHelper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GdipuActivityHelper PRIVATE
    campus_core
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::QuickDialogs2
    Qt6::WebEngineQuick
    Qt6::Widgets
    Qt6::Network
)
target_compile_definitions(GdipuActivityHelper PRIVATE QT_NO_CAST_TO_ASCII)
