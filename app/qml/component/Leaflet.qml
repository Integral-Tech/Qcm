pragma ComponentBehavior: Bound
import QtQml.Models
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Qcm.Material as MD

Item {
    // fold:
    // split: 1, stack: 1
    // split: 1, stack: 2
    // unfold:
    // split: 2, stack: 1

    id: root

    property bool rightAbove: false
    readonly property bool folded: state === 'fold'
    readonly property Item topItem: rightAbove ? rightPage : leftPage
    readonly property Item bottomItem: rightAbove ? leftPage : rightPage

    required property int leftMin
    required property int rightMin
    required property Item leftPage
    required property Item rightPage

    // save size when fold
    property int _bottomHeight: 0
    property int _bottomWidth: 0

    property var foldState: null

    states: [
        State {
            name: "fold"
            ParentChange {
                target: root.topItem
                parent: root
                width: parent.width
                height: parent.height
                x: 0
                y: 0
            }
            ParentChange {
                target: root.bottomItem
                parent: m_hide
                width: parent.width
                height: parent.height
                x: 0
                y: 0
            }
            PropertyChanges {
                root.topItem.visible: true
                root.bottomItem.visible: false
                split.handleOpacity: 0
            }
        },
        State {
            name: "unfold"
            ParentChange {
                target: root.rightPage
                parent: m_split_right
                width: parent.width
                height: parent.height
                x: 0
                y: 0
            }
            ParentChange {
                target: root.leftPage
                parent: m_split_left
                width: parent.width
                height: parent.height
                x: 0
                y: 0
            }
            PropertyChanges {
                root.topItem.visible: true
                root.bottomItem.visible: true
                split.handleOpacity: 1
            }
        }
    ]

    transitions: [
        Transition {
            to: "fold"
            SequentialAnimation {
                PropertyAction {
                    property: "handleOpacity"
                }
                ParentAnimation {
                    via: root
                    NumberAnimation {
                        properties: "x,y"
                        duration: 250
                    }
                    NumberAnimation {
                        properties: "width,height"
                        duration: 400
                    }
                }
                NumberAnimation {
                    target: root.bottomItem
                    property: "visible"
                    duration: 0
                }
            }
        },
        Transition {
            id: m_unfold_trans
            to: "unfold"
            SequentialAnimation {
                ParentAnimation {
                    via: root
                    NumberAnimation {
                        properties: "x,y"
                        duration: 200
                    }
                    NumberAnimation {
                        properties: "width,height"
                        duration: 0
                    }
                }

                NumberAnimation {
                    property: "handleOpacity"
                    duration: 40
                }
            }
        }
    ]

    function doFold() {
        _bottomHeight = bottomItem.height;
        _bottomWidth = bottomItem.width;
    }
    function doUnfold() {
    }

    function layout() {
        // need a valid left width
        // as split no busy prop, check manunlly
        //if (!folded && width >= leftPage.width + rightMin)
        //    m_left_fold_width = leftPage.width;

        const lw = leftPage.implicitWidth;
        const rw = rightPage.implicitWidth;
        let newState = width < Math.max(leftMin, lw) + Math.max(rightMin + rw) ? "fold" : "unfold";
        if (newState === state)
            return;
        if (newState == "fold") {
            doFold();
        } else {
            doUnfold();
        }
        root.state = newState;
    }

    Item {
        id: m_hide
        width: root._bottomWidth
        height: root._bottomHeight
        x: root.rightAbove ? -width : width
    }

    Timer {
        id: m_layout_timer
        interval: 100
        onTriggered: {
            if (root.visible)
                root.layout();
        }
    }
    onWidthChanged: {
        if (state === "")
            layout();
        m_layout_timer.start();
    }

    MD.SplitView {
        id: split
        anchors.fill: parent

        Item {
            id: m_split_left
            SplitView.minimumWidth: Math.max(root.leftMin, root.leftPage.implicitWidth)
        }

        Item {
            id: m_split_right
            SplitView.minimumWidth: Math.max(root.rightMin, root.rightPage.implicitWidth)
        }
    }

    // workaround for animation not move to right place
    Binding {
        when: root.state === 'unfold' && !m_unfold_trans.running 
        restoreMode: Binding.RestoreNone

        root.rightPage.width: m_split_right.width
        root.rightPage.height: m_split_right.height
        root.rightPage.x: 0
        root.rightPage.y: 0
    }

    Item {
        id: m_stack_holder
        anchors.fill: parent
    }
}
