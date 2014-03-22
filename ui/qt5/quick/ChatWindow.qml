/* Main Window */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

Window
{
    property alias textEntry: sendText
    signal sendMessage(string)

    title: "Chat"
    width: 600
    height: 400

    ColumnLayout {
        anchors.fill: parent
        ListView {
            id: list
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: input.top

            model: chatModel
            delegate: ChatMessage {}
            focus: true
            clip: true
        }
        RowLayout {
            id: input
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            Rectangle {
                id: rect
                border.width: 2
                border.color: "gray"
                radius: 3
                color: "transparent"
                anchors.left: parent.left
                anchors.right: sendButton.left
                height: input.height
            }
            TextInput {
                id: sendText
                anchors.fill: rect
                anchors.margins: 2

                focus: true
                inputMethodHints: Qt.ImhSensitiveData
                wrapMode: TextInput.Wrap
                onAccepted: { sendButton.clicked() }
            }
            Button {
                id: sendButton
                anchors.right: parent.right

                text: "Send"
                enabled: { sendText.text.length > 0 }
                onClicked: { sendMessage(sendText.text); sendText.text = "" }
            }
        }
    }
}
