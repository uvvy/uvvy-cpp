/* User card */
import QtQuick 2.1
import QtQuick.Controls 1.0

Component {
    Item {
        width: parent.width
        height: cardInfo.height
        Row {
            id: cardInfo
            spacing: 8
            Image {
                id: avatar
                source: avatarUrl
                width: 96
                height: 96
                smooth: true
            }
            Column {
                spacing: 3
                Text {
                    text: nickName
                    font.pixelSize: 24
                }
                Text {
                    text: firstName
                    font.pixelSize: 18
                }
                Text {
                    text: lastName
                    font.pixelSize: 14
                }
            }
            Column {
                Button {
                    text: "Call"
                }
            }
        }
    }
}
