import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.platform

import Qcm.App as QA
import Qcm.Service.Ncm as QNCM
import Qcm.Material as MD

MD.Page {
    id: root
    padding: 0
    title: qsTr("cloud")
    bottomPadding: radius

    MD.ListView {
        id: m_view
        anchors.fill: parent
        expand: true
        topMargin: 8
        bottomMargin: 8
        leftMargin: 24
        rightMargin: 24

        header: Item {
            width: parent.width
            implicitHeight: children[0].implicitHeight
            ColumnLayout {
                anchors.fill: parent
                anchors.bottomMargin: 8
                spacing: 4
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter

                    MD.IconButton {
                        action: QA.AppendListAction {
                            getSongs: function () {
                                const songs = [];
                                const model = qr_cloud.data;
                                for (let i = 0; i < model.rowCount(); i++) {
                                    songs.push(model.item(i).song);
                                }
                                return songs;
                            }
                        }
                    }
                    MD.IconButton {
                        action: Action {
                            icon.name: MD.Token.icon.upload
                            onTriggered: {
                                m_file_dialog.open();
                            }
                        }
                    }
                }
            }
        }
        model: qr_cloud.data
        delegate: QA.SongDelegate {
            width: ListView.view.contentWidth
            dgModel: model.song

            onClicked: {
                QA.App.playlist.switchTo(dgModel);
            }
        }

        QNCM.UserCloudQuerier {
            id: qr_cloud
        }
        Timer {
            id: timer_refresh

            property bool dirty: false

            function refreshSlot() {
                if (root.visible && dirty) {
                    if (qr_cloud.offset === 0)
                        qr_cloud.query();
                    else
                        qr_cloud.offset = 0;
                    dirty = false;
                }
            }

            interval: 15 * 60 * 1000
            repeat: true
            running: true

            Component.onCompleted: {
                root.visibleChanged.connect(refreshSlot);
                dirtyChanged.connect(refreshSlot);
            }
            onTriggered: dirty = true
        }
        MD.FAB {
            flickable: m_view
            action: Action {
                icon.name: MD.Token.icon.play_arrow

                onTriggered: {
                    const songs = [];
                    const model = qr_cloud.data;
                    for (let i = 0; i < model.rowCount(); i++) {
                        songs.push(model.item(i).song);
                    }
                    if (songs.length)
                        QA.App.playlist.switchList(songs);
                }
            }
        }
    }

    FileDialog {
        id: m_file_dialog
        onAccepted: {
            m_upload_api.upload(currentFile);
        }
    }

    QNCM.CloudUploadApi {
        id: m_upload_api
        onStatusChanged: {
            if (status === QA.enums.Finished) {
                qr_cloud.reset();
            }
        }
    }
}
