/* Call Window */
import QtQuick 2.0
import QtQuick.Window 2.1

Window {
    Rectangle {
        anchor.fill: parent

        GridLayout {
            // inside
            // user avatar on the left, 96x96 fixed
            Pixmap {
                width: 96
                height: 96
            }
            Text {
                text: "username"
            }
            Text {
                text: "call status"
            }
            // signal meter
            Widget {
                id: meter
            }
            Button {
                id: hangup
                text: "Hang up"
            }
        }
    }
}
