pragma Singleton
import QtQuick

// Single source of truth for colours, type and spacing. Everything in the UI reads
// from here, so light/dark switching is just `mode` changing.
QtObject {
    id: theme

    property string mode: "system"          // system | light | dark (bound from Main.qml)
    readonly property bool dark: mode === "dark"
                                 || (mode !== "light" && Application.styleHints.colorScheme === Qt.Dark)

    // ---- surfaces --------------------------------------------------------
    readonly property color bg: dark ? "#0f1519" : "#f3f6f8"
    readonly property color surface: dark ? "#182126" : "#ffffff"
    readonly property color surfaceAlt: dark ? "#1f2a30" : "#eef3f6"
    readonly property color surfaceHover: dark ? "#212e35" : "#f6f9fa"
    readonly property color border: dark ? "#2a373e" : "#dde6ea"
    readonly property color scrim: "#66000000"

    // sidebar stays deep teal in both modes: it is the brand anchor
    readonly property color sidebar: dark ? "#0b1719" : "#0c3b39"
    readonly property color sidebarText: "#c5dedb"
    readonly property color sidebarTextActive: "#ffffff"
    readonly property color sidebarActive: "#26ffffff"
    readonly property color sidebarHover: "#14ffffff"

    // ---- text ------------------------------------------------------------
    readonly property color text: dark ? "#e7eff2" : "#172b3a"
    readonly property color textMuted: dark ? "#9aacb5" : "#5d727d"
    readonly property color textFaint: dark ? "#71858f" : "#8595a0"

    // ---- brand + semantic --------------------------------------------------
    readonly property color primary: dark ? "#2bb3a7" : "#087d75"
    readonly property color primaryHover: dark ? "#43c6ba" : "#066a63"
    readonly property color primaryPressed: dark ? "#209a8f" : "#055852"
    readonly property color primarySoft: dark ? "#163a3d" : "#e3f3f1"
    readonly property color onPrimary: dark ? "#04201e" : "#ffffff"
    readonly property color danger: dark ? "#ff8f7f" : "#b3402f"
    readonly property color dangerSoft: dark ? "#3b201d" : "#fbeae7"
    readonly property color success: dark ? "#5fd39a" : "#18764d"
    readonly property color successSoft: dark ? "#123529" : "#e4f5ec"
    readonly property color info: dark ? "#82aaff" : "#3a5f9c"
    readonly property color infoSoft: dark ? "#182742" : "#e8effb"
    readonly property color neutralFg: dark ? "#a0aeb5" : "#6a777e"
    readonly property color neutralSoft: dark ? "#263238" : "#eceff1"

    // reminder cards (timetable)
    readonly property color reminderReg: dark ? "#f5cc55" : "#e5b322"
    readonly property color reminderRegSoft: dark ? "#3a2a18" : "#fff3e4"
    readonly property color reminderEvt: dark ? "#ff8686" : "#db5058"
    readonly property color reminderEvtSoft: dark ? "#422328" : "#fff0f1"
    readonly property color memoColor: dark ? "#6cd5a3" : "#279768"

    // five course colours: [background, accent]
    function course(i) {
        const light = [["#eaf0fd", "#6b8fd6"], ["#e5f4ef", "#4d9f88"], ["#fff1e4", "#cf9558"],
                       ["#f0eafa", "#9377bd"], ["#e6f3fa", "#5ea3c2"]]
        const night = [["#1a2540", "#7b9be0"], ["#15332c", "#56b39a"], ["#3a2c1b", "#d9a466"],
                       ["#2a2140", "#a68bd0"], ["#152f3c", "#6db5d3"]]
        return (dark ? night : light)[((i % 5) + 5) % 5]
    }

    // ---- shape & type ------------------------------------------------------
    readonly property int radiusSm: 6
    readonly property int radius: 10
    readonly property int radiusLg: 14

    readonly property string fontUi: "Microsoft YaHei UI"
    readonly property string fontIcon: "Segoe Fluent Icons"
    readonly property int textXs: 12
    readonly property int textSm: 13
    readonly property int textMd: 14
    readonly property int textLg: 16
    readonly property int textXl: 20
    readonly property int textTitle: 26
    readonly property int textMetric: 28

    // Segoe Fluent Icons / MDL2 code points
    readonly property var icon: ({
        search: "", refresh: "", stop: "", download: "", save: "",
        globe: "", calendar: "", list: "", settings: "", back: "",
        close: "", add: "", remove: "", bell: "", clock: "",
        info: "", check: "", warning: "", error: "", chevronDown: "",
        chevronLeft: "", chevronRight: "", folder: "", openWindow: "",
        place: "", school: "", edit: "", filter: "", inbox: "",
        more: "", sun: "", moon: "", login: ""
    })
}
