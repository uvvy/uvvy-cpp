/* Call Window */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Window 2.1

Window {
    property string avatarUrl: "http://gravatar.com/avatar/29191c38b8d1e4d6a3a25846c5e673c9?s=96"
    property string userName: "berkus"
    property string callStatus: "RINGING..."
    property alias hangupButton: hangup

    title: "Call"

    width: { r.width < 200 ? 200 : r.width }
    height: { r.height < 100 ? 100 : r.height }
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
                text: userName
            }
            Text {
                text: callStatus
            }
            // signal meter
            //Component {
            //    id: meter
            //}
        }
    }
    Button {
        id: hangup
        text: "Hang up"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }
}
