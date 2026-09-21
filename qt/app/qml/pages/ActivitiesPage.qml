import QtCore
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Gdipu

Rectangle {
    id: page
    color: Theme.bg

    readonly property bool narrow: width < 1000
    readonly property int gutter: narrow ? 20 : 28
    readonly property real tableWidth: width - gutter * 2
    readonly property bool showTime: tableWidth > 960
    readonly property bool showPlace: tableWidth > 780
    readonly property int colReg: 118
    readonly property int colPlace: 150
    readonly property int colTime: 176
    readonly property int colQuota: 88
    readonly property int colStatus: 104

    readonly property var acts: app.activities
    readonly property var scraper: app.scraper
    readonly property bool hasData: acts.total > 0
    readonly property real progress: scraper.expected > 0 ? scraper.done / scraper.expected : -1

    readonly property var statusOptions: ["全部报名状态", "报名未开始", "报名时间内", "报名已截止", "名额已满", "读取失败"]
    readonly property var sortOptions: ["报名开始时间 ↑", "报名截止时间 ↑"]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.gutter
        anchors.bottomMargin: 20
        spacing: 16

        // ---- header ---------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                spacing: 2
                Text {
                    text: "未开始活动"
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    text: page.acts.collectedAt !== ""
                          ? "数据采集于 " + page.acts.collectedAt + "（北京时间） · 按报名开始时间排列"
                          : "汇总学校活动系统中所有未开始的活动，并标出报名状态"
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: app.autoCollectHours > 0 ? "自动采集 · " + app.autoCollectHours + "小时" : "自动采集"
                glyph: Theme.icon.clock
                onClicked: autoMenu.open()
                Menu {
                    id: autoMenu
                    y: parent.height + 4
                    MenuItem { text: "关闭自动采集"; onTriggered: app.autoCollectHours = 0 }
                    MenuItem { text: "每 3 小时"; onTriggered: app.autoCollectHours = 3 }
                    MenuItem { text: "每 6 小时"; onTriggered: app.autoCollectHours = 6 }
                    MenuItem { text: "每 12 小时"; onTriggered: app.autoCollectHours = 12 }
                    MenuItem { text: "每 24 小时"; onTriggered: app.autoCollectHours = 24 }
                    MenuSeparator {}
                    MenuItem { text: app.desktopReminders ? "关闭电脑提醒" : "开启电脑提醒"; onTriggered: app.desktopReminders = !app.desktopReminders }
                }
            }

            AppButton {
                visible: page.scraper.busy
                text: "停止"
                glyph: Theme.icon.stop
                variant: "danger"
                onClicked: page.scraper.stop()
            }
            AppButton {
                text: page.scraper.busy ? "采集中…" : (page.hasData ? "重新采集" : "开始采集")
                glyph: Theme.icon.refresh
                variant: "primary"
                enabled: !page.scraper.busy
                onClicked: app.startCollect()
            }
            AppButton {
                text: "导出 Excel"
                glyph: Theme.icon.download
                enabled: page.acts.shown > 0
                onClicked: {
                    exportDialog.currentFile = exportDialog.currentFolder + "/" + app.defaultExportName
                    exportDialog.open()
                }
            }
            AppButton {
                glyph: Theme.icon.more
                variant: "ghost"
                implicitWidth: 38
                leftPadding: 0
                rightPadding: 0
                onClicked: moreMenu.open()

                Menu {
                    id: moreMenu
                    y: parent.height + 4
                    width: 180
                    padding: 6
                    background: Rectangle {
                        radius: Theme.radius
                        color: Theme.surface
                        border.color: Theme.border
                    }
                    MenuItem {
                        text: "保存为网页…"
                        enabled: page.acts.total > 0
                        implicitHeight: 36
                        contentItem: Text {
                            text: parent.text
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textMd
                            color: Theme.text
                            verticalAlignment: Text.AlignVCenter
                            opacity: parent.enabled ? 1 : 0.4
                        }
                        background: Rectangle {
                            radius: Theme.radiusSm
                            color: parent.highlighted ? Theme.surfaceAlt : "transparent"
                        }
                        onTriggered: {
                            htmlDialog.currentFile = htmlDialog.currentFolder + "/活动汇总_" + app.dateToday() + ".html"
                            htmlDialog.open()
                        }
                    }
                }
            }
        }

        // ---- progress / notice --------------------------------------------------------
        ProgressBanner {
            objectName: "activityBanner"
            Layout.fillWidth: true
            message: app.notice
            running: page.scraper.busy
            progress: page.progress
            kind: app.noticeKind
        }

        // ---- metrics --------------------------------------------------------------------
        GridLayout {
            Layout.fillWidth: true
            columns: page.narrow ? 2 : 4
            columnSpacing: 14
            rowSpacing: 14
            MetricCard {
                Layout.fillWidth: true
                label: "未开始活动"
                value: page.acts.total
                icon: Theme.icon.list
            }
            MetricCard {
                Layout.fillWidth: true
                label: "尚未开放报名"
                value: page.acts.notStarted
                icon: Theme.icon.clock
                accent: Theme.info
                accentSoft: Theme.infoSoft
            }
            MetricCard {
                Layout.fillWidth: true
                label: "报名时间内"
                value: page.acts.open
                icon: Theme.icon.check
                accent: Theme.success
                accentSoft: Theme.successSoft
            }
            MetricCard {
                Layout.fillWidth: true
                label: "有名额上限"
                value: page.acts.limited
                icon: Theme.icon.filter
                accent: Theme.reminderReg
                accentSoft: Theme.reminderRegSoft
            }
        }

        // ---- filters ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            visible: page.hasData

            SearchField {
                Layout.fillWidth: true
                Layout.minimumWidth: 200
                placeholderText: "搜索活动名称、地点或发起组织"
                text: page.acts.searchText
                onTextChanged: page.acts.searchText = text
            }
            FilterCombo {
                model: page.statusOptions
                currentIndex: Math.max(0, page.statusOptions.indexOf(page.acts.statusFilter))
                onActivated: page.acts.statusFilter = currentIndex === 0 ? "" : page.statusOptions[currentIndex]
            }
            FilterCombo {
                id: typeCombo
                model: ["全部活动类型"].concat(page.acts.types)
                currentIndex: Math.max(0, model.indexOf(page.acts.typeFilter))
                onActivated: page.acts.typeFilter = currentIndex === 0 ? "" : model[currentIndex]
            }
            FilterCombo {
                implicitWidth: 156
                model: page.sortOptions
                currentIndex: page.acts.sortMode
                onActivated: page.acts.sortMode = currentIndex
            }
            Text {
                text: "显示 " + page.acts.shown + " / " + page.acts.total + " 项"
                font.family: Theme.fontUi
                font.pixelSize: Theme.textXs
                color: Theme.textMuted
                visible: !page.narrow
            }
        }

        // ---- table ----------------------------------------------------------------------------
        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            // column header
            Rectangle {
                id: header
                visible: list.count > 0
                width: parent.width
                height: 40
                radius: Theme.radius
                color: Theme.surfaceAlt
                // square the bottom corners
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: parent.radius; color: parent.color }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    spacing: 16
                    Text { Layout.fillWidth: true; text: "活动名称 / 发起组织"; font: page.headerFont; color: Theme.textMuted }
                    Text { Layout.preferredWidth: page.colReg; text: "报名时间"; font: page.headerFont; color: Theme.textMuted }
                    Text { visible: page.showPlace; Layout.preferredWidth: page.colPlace; text: "活动地点"; font: page.headerFont; color: Theme.textMuted }
                    Text { visible: page.showTime; Layout.preferredWidth: page.colTime; text: "活动时间"; font: page.headerFont; color: Theme.textMuted }
                    Text { Layout.preferredWidth: page.colQuota; text: "名额"; font: page.headerFont; color: Theme.textMuted }
                    Text { Layout.preferredWidth: page.colStatus; text: "状态"; font: page.headerFont; color: Theme.textMuted }
                }
            }

            ListView {
                id: list
                anchors.top: header.visible ? header.bottom : parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                clip: true
                model: page.acts
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: 3
                        color: Theme.textFaint
                        opacity: parent.active ? 0.7 : 0.35
                    }
                }

                delegate: Rectangle {
                    id: row
                    required property int index
                    required property string name
                    required property string organizer
                    required property string type
                    required property string regStart
                    required property string regEnd
                    required property string place
                    required property string activityTime
                    required property string capacity
                    required property string remaining
                    required property string status
                    required property string statusKind
                    required property string url
                    required property string error
                    required property string activityKey
                    required property bool claimed

                    width: ListView.view.width
                    height: Math.max(68, rowLayout.implicitHeight + 24)
                    color: rowHover.hovered ? Theme.surfaceHover : "transparent"

                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        height: 1
                        color: Theme.border
                        opacity: 0.7
                    }

                    HoverHandler { id: rowHover; cursorShape: row.url !== "" ? Qt.PointingHandCursor : Qt.ArrowCursor }
                    TapHandler { enabled: row.url !== ""; onTapped: app.openActivity(row.url) }

                    RowLayout {
                        id: rowLayout
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 20
                        anchors.rightMargin: 20
                        spacing: 16

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 3
                            Text {
                                Layout.fillWidth: true
                                text: row.name
                                wrapMode: Text.Wrap
                                maximumLineCount: 2
                                elide: Text.ElideRight
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textMd
                                font.weight: Font.DemiBold
                                color: rowHover.hovered && row.url !== "" ? Theme.primary : Theme.text
                            }
                            Text {
                                Layout.fillWidth: true
                                text: (row.type || "类型未提供") + " · " + (row.organizer || "组织未提供")
                                elide: Text.ElideRight
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textXs
                                color: Theme.textMuted
                            }
                            Text {
                                Layout.fillWidth: true
                                visible: row.error !== ""
                                text: row.error
                                wrapMode: Text.Wrap
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textXs
                                color: Theme.danger
                            }
                        }
                        ColumnLayout {
                            Layout.preferredWidth: page.colReg
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2
                            Text {
                                text: row.regStart || "未获取"
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textSm
                                font.weight: Font.Medium
                                color: Theme.text
                            }
                            Text {
                                text: row.regEnd ? "至 " + row.regEnd : ""
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textXs
                                color: Theme.textMuted
                            }
                        }
                        Text {
                            visible: page.showPlace
                            Layout.preferredWidth: page.colPlace
                            Layout.alignment: Qt.AlignVCenter
                            text: row.place || "未提供"
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textSm
                            color: Theme.text
                        }
                        Text {
                            visible: page.showTime
                            Layout.preferredWidth: page.colTime
                            Layout.alignment: Qt.AlignVCenter
                            text: row.activityTime || "未提供"
                            wrapMode: Text.Wrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textSm
                            color: Theme.text
                        }
                        ColumnLayout {
                            Layout.preferredWidth: page.colQuota
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 2
                            Text {
                                text: row.capacity || "未获取"
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textLg
                                font.weight: Font.DemiBold
                                color: Theme.text
                            }
                            Text {
                                text: "剩余 " + (row.remaining || "未获取")
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.textXs
                                color: Theme.textMuted
                            }
                        }
                        ColumnLayout {
                            Layout.preferredWidth: page.colStatus
                            Layout.alignment: Qt.AlignVCenter
                            spacing: 4
                            StatusBadge {
                                id: badge
                                kind: row.statusKind
                                text: row.status
                            }
                            Button {
                                text: row.claimed ? "✓ 已抢到" : "标记已抢到"
                                implicitHeight: 28
                                font.pixelSize: 12
                                onClicked: page.acts.setClaimed(row.activityKey, !row.claimed)
                            }
                        }
                    }
                }
            }

            // ---- empty states ----------------------------------------------------------------
            EmptyState {
                anchors.centerIn: parent
                visible: !page.hasData
                icon: Theme.icon.inbox
                title: "还没有活动数据"
                subtitle: "先在「学校登录」页完成统一认证，再回来点击「开始采集」。程序只读取活动信息，不会自动报名。"
                AppButton {
                    text: "学校登录"
                    glyph: Theme.icon.globe
                    onClicked: app.openSchool()
                }
                AppButton {
                    text: "开始采集"
                    glyph: Theme.icon.refresh
                    variant: "primary"
                    enabled: !page.scraper.busy
                    onClicked: app.startCollect()
                }
            }
            EmptyState {
                anchors.centerIn: parent
                visible: page.hasData && page.acts.shown === 0
                icon: Theme.icon.search
                title: "没有符合条件的活动"
                subtitle: "换个关键词，或者清除筛选条件试试。"
                AppButton {
                    text: "清除筛选"
                    onClicked: page.acts.clearFilters()
                }
            }
        }
    }

    readonly property font headerFont: Qt.font({family: Theme.fontUi, pixelSize: Theme.textXs, weight: Font.DemiBold})

    FileDialog {
        id: exportDialog
        title: "导出 Excel"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Excel 工作簿 (*.xlsx)"]
        defaultSuffix: "xlsx"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: app.exportExcel(selectedFile)
    }
    FileDialog {
        id: htmlDialog
        title: "保存网页"
        fileMode: FileDialog.SaveFile
        nameFilters: ["网页文件 (*.html)"]
        defaultSuffix: "html"
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        onAccepted: app.exportHtml(selectedFile)
    }
}
