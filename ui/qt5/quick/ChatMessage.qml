/* Chat message delegate */
import QtQuick 2.1
import QtQuick.Controls 1.0

Component {
    Item {
        width: parent.width
        height: message.height

        Rectangle {
            id: line
            anchors { left: parent.left; right: parent.right }
            height: 1
            color: "lightgray"
        }

        Row {
            id: message
            spacing: 8
            Image {
                id: avatar
                source: avatarUrl
                width: 48
                height: 48
                smooth: true
            }
            Column {
                spacing: 3
                Text {
                    text: author
                    font.pixelSize: 10
                }
                Text {
                    text: date
                    font.pixelSize: 8
                }
                Text {
                    text: messageText
                    font.pixelSize: 17
                }
            }
        }
    }
}
