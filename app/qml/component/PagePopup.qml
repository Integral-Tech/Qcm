import QtQuick
import QtQuick.Controls as QC
import QtQuick.Window

import Qcm.Material as MD

MD.Popup {
    id: root
    property bool fillHeight: false
    property bool fillWidth: false
    property var props: Object()
    required property string source

    parent: QC.Overlay.overlay
    width: Math.min(400, parent.width)
    height: Math.min(implicitHeight, parent.height * 0.8)
    modal: true

    mdState.textColor: MD.Token.color.on_surface
    mdState.backgroundColor: MD.Token.color.surface

    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)

    Binding on height {
        value: root.parent.height
        when: root.fillHeight
    }
    Binding on width {
        value: root.parent.width
        when: root.fillWidth
    }

    // use attch from parent, tested
    readonly property bool isCompact: root.parent.Window.window?.windowClass === MD.Enum.WindowClassCompact

    Binding {
        when: root.isCompact
        root.fillHeight: true
        root.fillWidth: true
        root.padding: 0
        root.verticalPadding: 0
    }

    QtObject {
        id: m_page_context
        property int radius: root.isCompact ? 0 : MD.Token.shape.corner.large
        property QC.Action barAction: QC.Action {
            icon.name: MD.Token.icon.arrow_back
            onTriggered: {
                if(loader.item?.canBack) loader.item.back();
                else {
                    root.close();
                }
            }
        }
    }

    onSourceChanged: {
        props.pageContext = m_page_context;
        loader.setSource(source, props);
    }

    contentItem: Loader {
        id: loader
        asynchronous: false
    }

    background: Item {}

    closePolicy: QC.Popup.CloseOnEscape | QC.Popup.CloseOnPressOutside
}
