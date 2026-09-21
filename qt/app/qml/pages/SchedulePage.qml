import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Gdipu

Rectangle {
    id: page
    color: Theme.bg

    readonly property bool narrow: width < 1000
    readonly property int gutter: narrow ? 20 : 28
    readonly property var sched: app.schedule
    readonly property var importer: app.importer
    readonly property var scraper: app.scraper
    readonly property var weekOptions: {
        const values = ["全部周次"]
        for (let week = 1; week <= 30; ++week) values.push("第 " + week + " 周")
        return values
    }
    readonly property int rev: sched.revision          // touch to re-evaluate card bindings
    readonly property int labelWidth: narrow ? 74 : 96

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: page.gutter
        anchors.bottomMargin: 20
        spacing: 14

        // ---- header ---------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ColumnLayout {
                spacing: 2
                Text {
                    text: "我的课表"
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textTitle
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                Text {
                    text: {
                        const parts = []
                        if (page.sched.semester !== "") parts.push("学期：" + page.sched.semester)
                        if (page.sched.importedAt !== "") parts.push("上次导入：" + page.sched.importedAt)
                        return parts.length ? parts.join(" · ") : "从教务系统导入，也可手动补充临时调课"
                    }
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.textMuted
                }
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: "教务登录"
                glyph: Theme.icon.login
                variant: "ghost"
                onClicked: app.openAcademic()
            }
            AppButton {
                text: "设置日期"
                glyph: Theme.icon.calendar
                onClicked: dateDialog.openDialog()
            }
            AppButton {
                text: "新增课程"
                glyph: Theme.icon.add
                onClicked: courseDialog.openDialog()
            }
            AppButton {
                text: page.scraper.busy ? "采集中…" : "刷新活动"
                glyph: Theme.icon.refresh
                enabled: !page.scraper.busy
                onClicked: app.startCollect()
            }
            AppButton {
                text: page.importer.busy ? "导入中…" : "一键导入课表"
                glyph: Theme.icon.download
                variant: "primary"
                enabled: !page.importer.busy
                onClicked: page.importer.import()
            }
        }

        // ---- week bar --------------------------------------------------------------------
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            RowLayout {
                spacing: 8
                Text {
                    text: "显示周次"
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.textSm
                    color: Theme.textMuted
                }
                FilterCombo {
                    id: weekCombo
                    Layout.preferredWidth: 150
                    model: page.weekOptions
                    currentIndex: page.sched.week
                    onActivated: page.sched.week = currentIndex
                }
            }

            Text {
                text: "活动时间"
                font.family: Theme.fontUi
                font.pixelSize: Theme.textSm
                color: Theme.textMuted
            }
            FilterCombo {
                Layout.preferredWidth: 190
                model: ["报名后显示", "提前显示全部"]
                currentIndex: page.sched.activityReminderMode
                onActivated: page.sched.activityReminderMode = currentIndex
            }

            Text {
                text: page.rev >= 0 ? page.sched.summary : ""
                font.family: Theme.fontUi
                font.pixelSize: Theme.textSm
                color: Theme.textMuted
            }

            Item { Layout.fillWidth: true }

            // legend
            Row {
                spacing: 14
                visible: !page.narrow
                Repeater {
                    model: [{c: Theme.reminderReg, t: "报名"}, {c: Theme.reminderEvt, t: "活动"}, {c: Theme.memoColor, t: "备忘录"}]
                    delegate: Row {
                        required property var modelData
                        spacing: 6
                        Rectangle { width: 10; height: 10; radius: 3; color: modelData.c; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            text: modelData.t
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.textXs
                            color: Theme.textMuted
                        }
                    }
                }
            }
        }

        ProgressBanner {
            Layout.fillWidth: true
            message: page.scraper.busy ? app.notice
                     : (page.importer.busy || page.importer.message !== "" ? page.importer.message
                     : (page.sched.hasAnchor ? "" : "先点「设置日期」，告诉我任意一周的任意一天，就能显示每天的日期和活动提醒。")
                       )
            running: page.importer.busy || page.scraper.busy
            kind: !page.importer.busy && app.noticeKind === "error" && page.importer.message === app.notice ? "error" : "info"
        }

        // ---- timetable grid ------------------------------------------------------------------
        Card {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ScrollView {
                id: scroll
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                GridLayout {
                    id: grid
                    width: scroll.availableWidth
                    columns: 8
                    columnSpacing: 0
                    rowSpacing: 0

                    // header row
                    Item { Layout.preferredWidth: page.labelWidth; Layout.preferredHeight: 56 }
                    Repeater {
                        model: 7
                        delegate: Rectangle {
                            id: dayHead
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredWidth: 100
                            Layout.minimumWidth: 40
                            Layout.preferredHeight: 56
                            color: Theme.surfaceAlt
                            Column {
                                anchors.centerIn: parent
                                spacing: 1
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.sched.dayNames[dayHead.index]
                                    font.family: Theme.fontUi
                                    font.pixelSize: Theme.textSm
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.rev >= 0 ? (page.sched.week > 0 ? page.sched.dayDate(dayHead.index + 1) : "") : ""
                                    font.family: Theme.fontUi
                                    font.pixelSize: Theme.textXs
                                    color: Theme.primary
                                }
                            }
                        }
                    }

                    // six period rows
                    Repeater {
                        model: 6 * 8
                        delegate: Item {
                            id: cell
                            required property int index
                            readonly property int block: Math.floor(index / 8)
                            readonly property int col: index % 8
                            readonly property var cards: col === 0 || page.rev < 0 ? [] : page.sched.cards(col, block)

                            Layout.fillHeight: true
                            Layout.fillWidth: col > 0
                            Layout.preferredWidth: col === 0 ? page.labelWidth : 100
                            Layout.minimumWidth: col === 0 ? page.labelWidth : 40
                            Layout.preferredHeight: col === 0 ? 112 : Math.max(112, cellColumn.implicitHeight + 16)

                            // grid lines
                            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.border }
                            Rectangle {
                                visible: cell.col > 0
                                anchors.left: parent.left
                                height: parent.height
                                width: 1
                                color: Theme.border
                            }

                            // period label
                            Column {
                                visible: cell.col === 0
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.sched.periods[cell.block].label
                                    font.family: Theme.fontUi
                                    font.pixelSize: Theme.textXs
                                    font.weight: Font.DemiBold
                                    color: Theme.text
                                }
                                Text {
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.sched.periods[cell.block].time
                                    font.family: Theme.fontUi
                                    font.pixelSize: 11
                                    color: Theme.textFaint
                                }
                                Text {
                                    visible: !page.narrow
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    text: page.sched.periods[cell.block].group
                                    font.family: Theme.fontUi
                                    font.pixelSize: 11
                                    color: Theme.primary
                                }
                            }

                            Column {
                                id: cellColumn
                                visible: cell.col > 0
                                x: 5
                                y: 8
                                width: parent.width - 10
                                spacing: 5
                                Repeater {
                                    model: cell.cards
                                    delegate: Row {
                                        required property var modelData
                                        id: slotCard
                                        width: cellColumn.width
                                        spacing: 3
                                        CourseCard {
                                            width: parent.width - (marks.visible ? marks.width + 3 : 0)
                                            info: slotCard.modelData
                                            onClicked: slotDialog.openFor(cell.col, cell.block)
                                            onRemoveRequested: removeDialog.askFor(slotCard.modelData)
                                        }
                                        Column {
                                            id: marks
                                            width: 26
                                            spacing: 2
                                            visible: !!slotCard.modelData.registrationCount || !!slotCard.modelData.eventCount || !!slotCard.modelData.hasMemo
                                            BookmarkButton {
                                                visible: !!slotCard.modelData.registrationCount
                                                tint: Theme.reminderReg
                                                text: "报名提醒（" + (slotCard.modelData.registrationCount || 0) + "）"
                                                onClicked: slotDialog.openFor(cell.col, cell.block)
                                            }
                                            BookmarkButton {
                                                visible: !!slotCard.modelData.eventCount
                                                tint: Theme.reminderEvt
                                                text: "活动提醒（" + (slotCard.modelData.eventCount || 0) + "）"
                                                onClicked: slotDialog.openFor(cell.col, cell.block)
                                            }
                                            BookmarkButton {
                                                visible: !!slotCard.modelData.hasMemo
                                                tint: Theme.memoColor
                                                text: "查看或编辑备忘录"
                                                onClicked: slotDialog.openFor(cell.col, cell.block)
                                            }
                                        }
                                    }
                                }
                                AppButton {
                                    width: parent.width
                                    visible: cell.cards.length === 0 && page.sched.week > 0
                                    text: "+ 备忘"
                                    variant: "ghost"
                                    onClicked: slotDialog.openFor(cell.col, cell.block)
                                }
                            }
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: !page.sched.hasCourses && page.sched.week === 0
                icon: Theme.icon.calendar
                title: "还没有课表"
                subtitle: "先点「教务登录」登录教务系统，再点「一键导入课表」；也可以直接新增临时课程。"
                AppButton {
                    text: "教务登录"
                    glyph: Theme.icon.login
                    onClicked: app.openAcademic()
                }
                AppButton {
                    text: "一键导入课表"
                    glyph: Theme.icon.download
                    variant: "primary"
                    enabled: !page.importer.busy
                    onClicked: page.importer.import()
                }
            }
        }
    }

    DateDialog { id: dateDialog }
    CourseDialog { id: courseDialog }
    CourseDetailDialog { id: detailDialog }
    SlotDialog { id: slotDialog }

    // ---- confirm delete -------------------------------------------------------------------
    Dialog {
        id: removeDialog
        property var target: ({})
        function askFor(info) { target = info; open() }

        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        width: 380
        padding: 24
        background: Rectangle { radius: Theme.radiusLg; color: Theme.surface; border.color: Theme.border }
        Overlay.modal: Rectangle { color: Theme.scrim }

        contentItem: ColumnLayout {
            spacing: 10
            Text {
                text: "删除手动课程"
                font.family: Theme.fontUi
                font.pixelSize: Theme.textXl
                font.weight: Font.DemiBold
                color: Theme.text
            }
            Text {
                Layout.fillWidth: true
                text: "确定要删除「" + (removeDialog.target.title || "") + "」吗？此操作不会影响教务系统中的课表。"
                wrapMode: Text.Wrap
                font.family: Theme.fontUi
                font.pixelSize: Theme.textMd
                color: Theme.textMuted
            }
            RowLayout {
                Layout.topMargin: 12
                Layout.alignment: Qt.AlignRight
                spacing: 10
                AppButton { text: "取消"; onClicked: removeDialog.close() }
                AppButton {
                    text: "删除"
                    variant: "danger"
                    onClicked: {
                        page.sched.removeManualCourse(removeDialog.target.id)
                        removeDialog.close()
                    }
                }
            }
        }
    }
}
