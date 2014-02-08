/* User card */
import QtQuick 2.1

Component {
    id: userCardDelegate
    Item {
        height: 50
        Column {
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
