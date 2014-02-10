/* User card */
import QtQuick 2.1

Component {
    Item {
        width: parent.width
        height: cardInfo.height
        Column {
            id: cardInfo
            Text {
                text: name
                font.pixelSize: 24
            }
            Text {
                text: nick
                font.pixelSize: 12
            }
        }
    }
}
