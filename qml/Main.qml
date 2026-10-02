import QtQuick
import QtQuick.Controls

Window {
    id: root
    width: 960
    height: 740
    visible: true
    title: qsTr("中国象棋")
    color: "#2e221a"
    minimumWidth: 820
    minimumHeight: 660

    property int cell: 64
    property int boardW: 9 * cell
    property int boardH: 10 * cell
    property int boardLeft: (width - boardW) / 2
    property int boardTop: 90

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
        x: boardLeft - 18
        y: boardTop - 18
        width: boardW + 36
        height: boardH + 36
        color: "#e6c987"
        border { color: "#8b5a2b"; width: 5 }
        radius: 10

        // 棋盘线
        Canvas {
            anchors { fill: parent; margins: 18 }
            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();
                const w = boardW, h = boardH;
                ctx.strokeStyle = "#5a3a1a";
                ctx.lineWidth = 1.5;

                // 横线 10 条
                for (let r = 0; r < 10; ++r) {
                    ctx.beginPath();
                    ctx.moveTo(0, r * cell);
                    ctx.lineTo(w, r * cell);
                    ctx.stroke();
                }
                // 竖线 9 条（河界处断开，两边边框保留）
                for (let c = 0; c < 9; ++c) {
                    ctx.beginPath();
                    ctx.moveTo(c * cell, 0);
                    ctx.lineTo(c * cell, 4 * cell);
                    ctx.stroke();
                    ctx.beginPath();
                    ctx.moveTo(c * cell, 5 * cell);
                    ctx.lineTo(c * cell, h);
                    ctx.stroke();
                    // 河界两侧边框
                    if (c === 0 || c === 8) {
                        ctx.beginPath();
                        ctx.moveTo(c * cell, 4 * cell);
                        ctx.lineTo(c * cell, 5 * cell);
                        ctx.stroke();
                    }
                }
                // 河界文字
                ctx.fillStyle = "#5a3a1a";
                ctx.font = "28px serif";
                ctx.textAlign = "center";
                ctx.fillText("楚 河 · 汉 界", w / 2, 4.6 * cell);

                // 九宫斜线（红方下，黑方上）
                ctx.beginPath();
                ctx.moveTo(3 * cell, 0); ctx.lineTo(5 * cell, 2 * cell);
                ctx.moveTo(5 * cell, 0); ctx.lineTo(3 * cell, 2 * cell);
                ctx.moveTo(3 * cell, 7 * cell); ctx.lineTo(5 * cell, 9 * cell);
                ctx.moveTo(5 * cell, 7 * cell); ctx.lineTo(3 * cell, 9 * cell);
                ctx.stroke();
            }
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

                // 棋子
                Rectangle {
                    anchors.centerIn: parent
                    width: cell - 14
                    height: cell - 14
                    radius: width / 2
                    visible: controller.pieceAt(gridRow, gridCol) !== ""
                    color: {
                        // 简化：红方亮色底、黑方暗色底（通过文字区分）
                        return "#f5ead0"
                    }
                    border { width: 2; color: "#5a3a1a" }

                    // 选中高亮
                    Rectangle {
                        anchors.fill: parent
                        radius: parent.radius
                        color: "transparent"
                        border { width: 3; color: "#f6c445" }
                        visible: controller.isSelected(gridRow, gridCol)
                    }
                    // 合法目标标记
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width * 0.28
                        height: width
                        radius: width / 2
                        color: "#e07b39"
                        opacity: 0.85
                        visible: controller.isLegalTarget(gridRow, gridCol) &&
                                 controller.pieceAt(gridRow, gridCol) === ""
                    }
                    Rectangle {
                        anchors.centerIn: parent
                        width: parent.width - 8
                        height: parent.height - 8
                        radius: width / 2
                        color: "transparent"
                        border { width: 3; color: "#e07b39" }
                        opacity: 0.85
                        visible: controller.isLegalTarget(gridRow, gridCol) &&
                                 controller.pieceAt(gridRow, gridCol) !== ""
                    }

                    Text {
                        anchors.centerIn: parent
                        text: controller.pieceAt(gridRow, gridCol)
                        font { pixelSize: cell * 0.42; bold: true }
                        color: {
                            const ch = text.charCodeAt(0);
                            // 黑方传统字（将士象馬車砲卒）用黑，红方用红
                            return ["将","士","象","馬","車","砲","卒"].indexOf(text) >= 0
                                   ? "#111111" : "#b32020"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
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
