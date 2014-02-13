/* Main Window */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

Window
{
    title: "Chat"
    width: 600
    height: 400

    ColumnLayout {
        anchors.fill: parent
        ListView {
            anchors.fill: parent
            model: ChatModel {}
            delegate: ChatMessage {}
            focus: true
            clip: true
        }
    }
}
