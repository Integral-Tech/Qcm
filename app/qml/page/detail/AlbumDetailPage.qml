pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Qcm.App as QA
import Qcm.Material as MD

MD.Page {
    id: root

    readonly property var album: qr_al.data.album
    property QA.item_id itemId
    title: qsTr("album")

    padding: 0
    scrolling: !m_view.atYBeginning

    MD.FlickablePane {
        id: m_view_pane
        view: m_view
        excludeBegin: m_view.headerItem.height - m_control_pane.height + view.topMargin
        radius: root.radius
        bottomMargin: MD.MatProp.size.verticalPadding
    }

    MD.ListView {
        id: m_view
        anchors.fill: parent
        reuseItems: true
        contentY: 0

        topMargin: MD.MatProp.size.verticalPadding
        bottomMargin: MD.MatProp.size.verticalPadding * 2

        model: qr_al.data

        readonly property bool single: width < m_cover.displaySize.width * (1.0 + 1.5) + 8

        Item {
            visible: false

            QA.Image {
                id: m_cover
                Layout.preferredWidth: displaySize.width
                Layout.preferredHeight: displaySize.height

                displaySize: Qt.size(240, 240)
                elevation: MD.Token.elevation.level2
                source: QA.Util.image_url(root.album.itemId)
                radius: 16
            }
            MD.Text {
                id: m_title
                maximumLineCount: 2
                text: root.album.name
                typescale: m_view.single ? MD.Token.typescale.headline_medium : MD.Token.typescale.headline_large
            }
            RowLayout {
                id: m_info
                spacing: 12
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: `${root.album.trackCount} tracks`
                }
                MD.Text {
                    typescale: MD.Token.typescale.body_medium
                    text: QA.Util.formatDateTime(root.album.publishTime, 'yyyy.MM')
                }
            }
            MD.Text {
                id: m_artist
                typescale: MD.Token.typescale.body_medium
                text: QA.Util.joinName(m_view.model.extra?.artists, '/')
                /*
                        onClicked: {
                            const artists = root.albumInfo.artists;
                            if (artists.length === 1)
                                QA.Action.route_by_id(artists[0].itemId);
                            else
                                MD.Util.show_popup('qrc:/Qcm/App/qml/component/ArtistsPopup.qml', {
                                        "model": artists
                                    });
                        }
                        */
            }

            QA.ListDescription {
                id: m_desc
                description: root.album.description.trim()
            }
            RowLayout {
                id: m_control_pane
                Layout.alignment: Qt.AlignHCenter

                MD.IconButton {
                    action: QA.AppendListAction {
                        getSongIds: function () {
                            return QA.Util.collect_ids(qr_al.data);
                        }
                    }
                }
                MD.IconButton {
                    id: btn_fav
                    action: QA.CollectAction {
                        itemId: root.itemId
                    }
                }
                MD.IconButton {
                    id: btn_comment
                    // TODO
                    visible: false
                    action: QA.CommentAction {
                        itemId: root.itemId
                    }
                }
            }
        }

        header: Item {
            width: parent.width
            implicitHeight: children[0].implicitHeight

            ColumnLayout {
                anchors.fill: parent
                spacing: 16

                MD.Pane {
                    Layout.fillWidth: true
                    radius: root.radius
                    padding: 16

                    ColumnLayout {
                        width: parent.width
                        spacing: 0
                        RowLayout {
                            spacing: 16
                            visible: !m_view.single

                            LayoutItemProxy {
                                target: m_cover
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignTop
                                spacing: 12
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    target: m_artist
                                }
                                LayoutItemProxy {
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            visible: m_view.single

                            LayoutItemProxy {
                                Layout.alignment: Qt.AlignHCenter
                                target: m_cover
                            }
                            MD.Space {
                                spacing: 16
                            }
                            ColumnLayout {
                                Layout.alignment: Qt.AlignHCenter
                                spacing: 12

                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.maximumWidth: implicitWidth
                                    Layout.fillWidth: true
                                    target: m_title
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    target: m_info
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.maximumWidth: implicitWidth
                                    Layout.fillWidth: true
                                    target: m_artist
                                }
                                LayoutItemProxy {
                                    Layout.alignment: Qt.AlignHCenter
                                    Layout.fillWidth: true
                                    visible: !!m_desc.description
                                    target: m_desc
                                }
                            }
                        }
                    }
                }

                LayoutItemProxy {
                    target: m_control_pane
                }
            }
        }
        delegate: QA.SongDelegate {
            required property int index
            required property var model

            width: ListView.view.contentWidth
            leftMargin: 16
            rightMargin: 16

            subtitle: QA.Util.joinName(QA.Store.extra(model.itemId)?.artists, '/')

            onClicked: {
                QA.Action.play_by_id(dgModel.itemId);
            }
        }

        footer: MD.ListBusyFooter {
            running: qr_al.querying
            width: ListView.view.contentWidth
        }
    }
    MD.FAB {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 16
        anchors.bottomMargin: 16
        flickable: m_view
        action: Action {
            icon.name: MD.Token.icon.play_arrow
            onTriggered: {
                QA.Action.switch_ids(QA.Util.collect_ids(qr_al.data));
            }
        }
    }

    QA.AlbumQuery {
        id: qr_al
        itemId: root.itemId.sid
        Component.onCompleted: reload()
    }
}
