import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Service.Ncm as QNcm
import Qcm.Material as MD

MD.Page {
    id: root
    title: qsTr('about')
    padding: 0
    readonly property var info: QA.App.info()

    MD.Flickable {
        id: flick
        anchors.fill: parent
        leftMargin: 24
        rightMargin: 24

        ColumnLayout {
            id: content

            height: implicitHeight
            width: parent.width
            spacing: 8

            MD.Image {
                property int size: 96
                MD.MatProp.elevation: MD.Token.elevation.level0
                Layout.preferredHeight: size
                Layout.preferredWidth: size
                Layout.alignment: Qt.AlignHCenter
                mipmap: true
                source: 'qrc:/Qcm/App/assets/Qcm.svg'
            }

            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.headline_small
                text: root.info.name
            }
            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.label_medium
                opacity: 0.8
                text: root.info.version
            }
            MD.Text {
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.label_large
                text: root.info.author
            }

            MD.Text {
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                Layout.alignment: Qt.AlignHCenter
                typescale: MD.Token.typescale.body_large
                text: root.info.summary
            }

            MD.DrawerItem {
                Layout.fillWidth: true
                icon.name: MD.Token.icon['public']
                text: qsTr('project website')
                font.capitalization: Font.Capitalize
                trailing: MD.Icon {
                    name: MD.Token.icon.launch
                }
                onClicked: {
                    Qt.openUrlExternally('https://github.com/hypengw/Qcm');
                }
            }
            MD.DrawerItem {
                Layout.fillWidth: true
                icon.name: MD.Token.icon.flag
                font.capitalization: Font.Capitalize
                text: qsTr('report an issue')
                onClicked: {
                    Qt.openUrlExternally('https://github.com/hypengw/Qcm/issues');
                }
                trailing: MD.Icon {
                    name: MD.Token.icon.launch
                }
            }
        }
    }

    footer: MD.Control {
        horizontalPadding: 24
        contentItem: RowLayout {
            Item {
                Layout.fillWidth: true
            }
            MD.Button {
                type: MD.Enum.BtText
                text: qsTr('close')
                onClicked: {
                    MD.Util.closePopup(root);
                }
            }
        }
    }
}
