import QtQuick
import QtWebEngine
import Gdipu

// One embedded browser, driven from C++ through a WebBridge.
Item {
    id: pane
    property var bridge: null
    property WebEngineProfile profile: null
    property alias view: web
    readonly property bool loading: web.loading
    readonly property int loadProgress: web.loadProgress
    readonly property string address: web.url.toString()

    WebEngineView {
        id: web
        anchors.fill: parent
        profile: pane.profile
        // never let Chromium freeze or discard a page the scraper is about to read
        lifecycleState: WebEngineView.LifecycleState.Active
        backgroundColor: Theme.surface

        onUrlChanged: if (pane.bridge) pane.bridge.currentUrl = url.toString()
        onLoadingChanged: function (info) {
            if (pane.bridge)
                pane.bridge.notifyLoad(info.status, info.url.toString(), info.errorString)
        }
        // Pop-ups (target=_blank, window.open) stay in this view, as in the old build.
        onNewWindowRequested: function (request) { request.openIn(web) }

        Component.onCompleted: if (pane.bridge) pane.bridge.attached = true
    }

    Connections {
        target: pane.bridge
        function onRunScript(id, js) {
            web.runJavaScript(js, function (result) { pane.bridge.scriptResult(id, result) })
        }
        function onLoadUrl(url) {
            if (web.url.toString() === url.toString())
                web.reload()
            else
                web.url = url
        }
        function onStopRequested() { web.stop() }
    }

    function goBack() { if (web.canGoBack) web.goBack() }
    function reload() { web.reload() }
}
