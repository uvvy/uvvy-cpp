/* Call Window */
import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Window 2.1

Window {
    Rectangle {
        anchors.fill: parent

        Grid {
            // inside
            // user avatar on the left, 96x96 fixed
            Image {
                width: 96
                height: 96
                source: avatarUrl
            }
            Text {
                text: "username"
            }
            Text {
                text: "call status"
            }
            // signal meter
            //Component {
            //    id: meter
            //}
            Button {
                id: hangup
                text: "Hang up"
            }
        }
    }
}
