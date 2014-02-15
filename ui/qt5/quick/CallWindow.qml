/* Call Window */
import QtQuick 2.0
import QtQuick.Controls 1.0
import QtQuick.Window 2.1

Window {
    width: r.width
    height: r.height
    Row {
        id: r
        spacing: 7
        // inside
        // user avatar on the left, 96x96 fixed
        Image {
            width: 96
            height: 96
            source: avatarUrl
        }
        Column {
            spacing: 5
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
