import QtQuick
import QtQuick.Controls
import Xiangqi 1.0

Window {
    id: root
    width: 960
    height: 800
    visible: true
    title: qsTr("中国象棋")
    color: "#2e221a"
    minimumWidth: 820
    minimumHeight: 786

    // 由 QML 直接创建的单例控制器（无注入时序问题）
    GameController {
        id: controller
    }

    property int cell: 64
    property int boardW: 9 * cell
    property int boardH: 10 * cell
    property int gridW: 8 * cell
    property int gridH: 9 * cell
    property int boardMargin: 28
    property int boardLeft: (width - gridW) / 2
    property int boardTop: 96

    // ---------- 顶部控制栏 ----------
    Rectangle {
        id: toolbar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 64
        color: "#1e1610"

        Row {
            anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
            spacing: 10

            Button {
                text: qsTr("新游戏（双人）")
                onClicked: controller.newGame(0)
            }
            Button {
                text: qsTr("新游戏（人机）")
                onClicked: controller.newGame(1)
            }
            Button {
                text: qsTr("悔棋")
                onClicked: controller.undoMove()
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("AI 深度：")
                color: "#d8c9a3"
            }
            SpinBox {
                from: 1; to: 5; value: 3
                onValueChanged: controller.setSearchDepth(value)
            }
        }

        Label {
            anchors { right: parent.right; rightMargin: 16; verticalCenter: parent.verticalCenter }
            text: controller.modeName
            color: "#b8a878"
            font.pixelSize: 14
        }
    }

    // ---------- 棋盘底板 ----------
    Rectangle {
        x: boardLeft - boardMargin
        y: boardTop - boardMargin
        width: gridW + 2 * boardMargin
        height: gridH + 2 * boardMargin
        color: "#e6c987"
        border { color: "#8b5a2b"; width: 5 }
        radius: 10

        // 棋盘内容容器（内缩 margin：交叉点(0,0)位于 boardLeft/boardTop，棋子完整在底板内）
        Rectangle {
            id: boardInner
            x: boardMargin
            y: boardMargin
            width: gridW
            height: gridH
            color: "transparent"

        // 棋盘线（纯 QML，确定性渲染）
        // 横线 10 条
        Repeater {
            model: 10
            Rectangle {
                required property int index
                x: 0
                y: index * cell
                width: gridW
                height: 1.5
                color: "#5a3a1a"
            }
        }
        // 竖线上段（0-4 行）
        Repeater {
            model: 9
            Rectangle {
                required property int index
                x: index * cell
                y: 0
                width: 1.5
                height: 4 * cell
                color: "#5a3a1a"
            }
        }
        // 竖线下段（5-9 行）
        Repeater {
            model: 9
            Rectangle {
                required property int index
                x: index * cell
                y: 5 * cell
                width: 1.5
                height: 4 * cell
                color: "#5a3a1a"
            }
        }
        // 河界两侧边框（第 1/9 列）
        Rectangle { x: 0; y: 4 * cell; width: 1.5; height: cell; color: "#5a3a1a" }
        Rectangle { x: 8 * cell; y: 4 * cell; width: 1.5; height: cell; color: "#5a3a1a" }
        // 九宫斜线（黑方上，红方下）
        Rectangle {
            x: 3 * cell; y: 0
            width: Math.sqrt(2) * 2 * cell
            height: 2
            rotation: 45
            transformOrigin: Item.TopLeft
            color: "#5a3a1a"
        }
        Rectangle {
            x: 5 * cell; y: 0
            width: Math.sqrt(2) * 2 * cell
            height: 2
            rotation: -45
            transformOrigin: Item.TopLeft
            color: "#5a3a1a"
        }
        Rectangle {
            x: 3 * cell; y: 7 * cell
            width: Math.sqrt(2) * 2 * cell
            height: 2
            rotation: 45
            transformOrigin: Item.TopLeft
            color: "#5a3a1a"
        }
        Rectangle {
            x: 5 * cell; y: 7 * cell
            width: Math.sqrt(2) * 2 * cell
            height: 2
            rotation: -45
            transformOrigin: Item.TopLeft
            color: "#5a3a1a"
        }
        // 河界文字
        Text {
            text: "楚 河 · 汉 界"
            color: "#5a3a1a"
            font { pixelSize: 28; family: "serif" }
            anchors.horizontalCenter: parent.horizontalCenter
            y: 4.6 * cell - 14
        }

        // 棋子层
        Repeater {
            model: 90
            Rectangle {
                required property int index
                property int gridRow: Math.floor(index / 9)
                property int gridCol: index % 9
                x: gridCol * cell
                y: gridRow * cell
                width: cell
                height: cell
                color: "transparent"

                // 棋子（中心在交叉点：格子左上角）
                Rectangle {
                    id: piece
                    property string pieceChar: (controller.version, controller.pieceAt(gridRow, gridCol))
                    x: -width / 2
                    y: -height / 2
                    width: cell - 14
                    height: cell - 14
                    radius: width / 2
                    visible: pieceChar !== ""
                    color: ["将","士","象","馬","車","砲","卒"].indexOf(pieceChar) >= 0
                            ? "#3a3028" : "#f5ead0"
                    border { width: 2; color: "#5a3a1a" }

                    Text {
                        anchors.centerIn: parent
                        text: piece.pieceChar
                        font { pixelSize: cell * 0.42; bold: true }
                        color: ["将","士","象","馬","車","砲","卒"].indexOf(piece.pieceChar) >= 0
                               ? "#f0e6d2" : "#b32020"
                    }
                }

                // 合法目标标记（空位，格子层）
                Rectangle {
                    x: -width / 2
                    y: -height / 2
                    width: cell * 0.28
                    height: width
                    radius: width / 2
                    color: "#e07b39"
                    opacity: 0.85
                    visible: (controller.version, controller.isLegalTarget(gridRow, gridCol) && controller.pieceAt(gridRow, gridCol) === "")
                }
                // 合法目标标记（吃子，格子层）
                Rectangle {
                    x: -width / 2
                    y: -height / 2
                    width: cell - 8
                    height: cell - 8
                    radius: width / 2
                    color: "transparent"
                    border { width: 3; color: "#e07b39" }
                    opacity: 0.85
                    visible: (controller.version, controller.isLegalTarget(gridRow, gridCol) && controller.pieceAt(gridRow, gridCol) !== "")
                }
                // 选中高亮（格子层）
                Rectangle {
                    x: -width / 2
                    y: -height / 2
                    width: cell - 10
                    height: cell - 10
                    radius: width / 2
                    color: "transparent"
                    border { width: 3; color: "#f6c445" }
                    visible: (controller.version, controller.isSelected(gridRow, gridCol))
                }

                // 点击区：中心在交叉点，56x56 小于格距 64，避免相邻格误命中
                MouseArea {
                    x: -28
                    y: -28
                    width: 56
                    height: 56
                    onClicked: controller.onSquareClicked(gridRow, gridCol)
                }
            }
        }
        }
    }

    // ---------- 底部状态栏 ----------
    Rectangle {
        id: statusbar
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 56
        color: "#1e1610"

        Label {
            anchors { left: parent.left; leftMargin: 20; verticalCenter: parent.verticalCenter }
            text: controller.statusText
            color: controller.gameOver ? "#f6c445" : "#e8dcb8"
            font { pixelSize: 20; bold: true }
        }
    }
}
