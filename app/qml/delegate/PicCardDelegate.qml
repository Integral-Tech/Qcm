import QtQuick
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Card {
    id: root

    property alias image: image
    property string subText
    property int picWidth: 160

    horizontalPadding: 0

    contentItem: ColumnLayout {
        QA.Image {
            id: image
            Layout.preferredWidth: displaySize.width
            Layout.preferredHeight: displaySize.height
            displaySize: Qt.size(root.picWidth, root.picWidth)
            fixedSize: false
            radius: root.background.radius
        }

        ColumnLayout {
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            Layout.fillWidth: true

            MD.Label {
                id: label
                Layout.fillWidth: true
                text: root.text
                maximumLineCount: 2
                typescale: MD.Token.typescale.body_medium
            }

            MD.Label {
                id: label_sub
                Layout.alignment: Qt.AlignHCenter
                text: root.subText
                visible: !!text
                opacity: 0.6
                typescale: MD.Token.typescale.body_medium
                maximumLineCount: 1
            }
        }
    }
}
