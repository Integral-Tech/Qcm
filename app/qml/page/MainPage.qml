import QtCore
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls as QC

import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0

    property int pageIndex: -1
    showHeader: false

    title: m_page_stack.currentItem?.title ?? ""
    canBack: m_page_stack.canBack

    backgroundColor: MD.MatProp.backgroundColor
    radius: MD.Token.shape.corner.large

    function back() {
        m_page_stack.back();
    }

    Connections {
        function onRoute(msg) {
            m_page_stack.push_page(msg.url, msg.props);
        }

        target: QA.Action
    }
    contentItem: Item {
        RowLayout {
            id: m_large_layout
            anchors.fill: parent
            visible: false

            ColumnLayout {
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                Layout.preferredWidth: 56 + 24
                Layout.fillWidth: false
                Layout.fillHeight: true

                StackLayout {
                    Layout.fillHeight: false
                    currentIndex: 1

                    Binding on currentIndex {
                        value: 0
                        when: m_page_stack.depth > 1 || !!page_container.canBack
                    }

                    ColumnLayout {
                        MD.IconButton {
                            Layout.alignment: Qt.AlignHCenter
                            action: QC.Action {
                                icon.name: MD.Token.icon.arrow_back

                                onTriggered: {
                                    if (m_page_stack.depth > 1)
                                        m_page_stack.pop_page();
                                    else if (page_container.canBack)
                                        page_container.back();
                                }
                            }
                        }
                        Item {
                            implicitWidth: 56 + 24
                        }
                    }
                    MD.ListView {
                        Layout.fillWidth: true
                        implicitHeight: contentHeight
                        interactive: false
                        spacing: 12
                        reuseItems: false
                        currentIndex: root.pageIndex

                        header: ColumnLayout {
                            width: ListView.view.width
                            spacing: 0
                            MD.RailItem {
                                icon.name: MD.Token.icon.menu
                                text: qsTr('menu')
                                onClicked: {
                                    m_drawer.open();
                                }
                            }
                            Item {
                                implicitHeight: 12
                            }
                        }

                        delegate: MD.RailItem {
                            required property var model
                            required property int index

                            width: ListView.view.width
                            icon.name: MD.Token.icon[model.icon]
                            text: model.name
                            checked: root.pageIndex == index
                            onClicked: {
                                if (model.action)
                                    model.action.do();
                                else
                                    root.pageIndex = index;
                            }
                        }

                        model: {
                            const p = [...QA.Global.session.pages];
                            if (QA.App.debug) {
                                const page = QA.Util.create_page();
                                page.name = qsTr('test');
                                page.icon = 'queue_music';
                                page.source = 'qrc:/Qcm/App/qml/page/MaterialTest.qml';
                                p.push(page);
                            }
                            return p;
                        }

                        Component.onCompleted: {
                            root.pageIndex = 0;
                        }
                        onCurrentIndexChanged: {
                            const m = model[currentIndex];
                            if (m.source) {
                                page_container.switchTo(m.source, m.props ?? {}, m.cache);
                            }
                        }
                    }
                }
                Item {
                    Layout.fillHeight: true
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    action: QC.Action {
                        icon.name: MD.Token.icon.search
                        onTriggered: {
                            QA.Global.route('qrc:/Qcm/App/qml/page/SearchPage.qml');
                        }
                    }
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !QA.Global.use_system_color_scheme
                    action: QA.ColorSchemeAction {}
                }
                MD.IconButton {
                    Layout.alignment: Qt.AlignHCenter
                    action: QC.Action {
                        icon.name: MD.Token.icon.settings

                        onTriggered: {
                            QA.Action.popup_special(QA.enums.SRSetting);
                        }
                    }
                }
            }

            LayoutItemProxy {
                target: m_content
            }
        }
        ColumnLayout {
            id: m_small_layout
            visible: false
            anchors.fill: parent
            spacing: 0

            LayoutItemProxy {
                target: m_content
            }

            MD.Pane {
                Layout.fillWidth: true
                visible: m_page_stack.depth <= 1
                padding: 0
                backgroundColor: MD.MatProp.color.surface_container
                elevation: MD.Token.elevation.level2
                RowLayout {
                    anchors.fill: parent
                    Repeater {
                        model: QA.Global.session.pages.filter(el => el.primary)
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: 12 + children[0].implicitHeight + 16
                            MD.BarItem {
                                anchors.fill: parent
                                anchors.topMargin: 12
                                anchors.bottomMargin: 16
                                icon.name: modelData.icon
                                text: modelData.name
                                checked: root.pageIndex == index
                                onClicked: {
                                    if (modelData.action)
                                        modelData.action.do();
                                    else
                                        root.pageIndex = index;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    QA.Drawer {
        id: m_drawer
        width: Math.min(400, (root.Window.window?.width ?? 0) * 0.8)
        height: root.height
    }
    Item {
        visible: false
        ColumnLayout {
            id: m_content
            Layout.fillHeight: true
            Layout.fillWidth: true
            spacing: 0
            QA.PageStack {
                id: m_page_stack
                Layout.fillHeight: true
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                Layout.rightMargin: 16
                clip: true
                initialItem: QA.PageContainer {
                    id: page_container
                    initialItem: Item {}
                    property string title: currentItem?.title ?? ""
                    property int barType: MD.Enum.AppBarCenterAligned
                    property bool barVisible: Window.window?.windowClass === MD.Enum.WindowClassCompact
                    property QC.Action barAction: root.canBack ? m_back_action : m_draw_action
                }
                layer.enabled: true
                layer.effect: MD.RoundClip {
                    radius: root.radius
                    size: Qt.vector2d(m_page_stack.width, m_page_stack.height)
                }
                property QC.Action barAction: root.canBack ? m_back_action : null
                property bool barVisible: Window.window?.windowClass === MD.Enum.WindowClassCompact
                QC.Action {
                    id: m_back_action
                    icon.name: MD.Token.icon.arrow_back
                    onTriggered: {
                        if (root.canBack)
                            root.back();
                    }
                }
                QC.Action {
                    id: m_draw_action
                    icon.name: MD.Token.icon.menu
                    onTriggered: {
                        m_drawer.open();
                    }
                }

                Binding {
                    when: root.Window.window?.windowClass === MD.Enum.WindowClassCompact
                    m_page_stack.Layout.topMargin: 0
                    m_page_stack.Layout.bottomMargin: 0
                    m_page_stack.Layout.rightMargin: 0
                }
            }
            QA.PlayBar {
                Layout.fillWidth: true
            }
        }
    }

    Connections {
        target: root.Window.window
        function onWindowClassChanged() {
            if (this.target.windowClass === MD.Enum.WindowClassCompact) {
                m_small_layout.visible = true;
                m_large_layout.visible = false;
            } else {
                m_large_layout.visible = true;
                m_small_layout.visible = false;
            }
        }
        Component.onCompleted: {
            this.onWindowClassChanged();
        }
    }
}
