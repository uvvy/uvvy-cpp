/* Main Window */
import QtQuick 2.0
import QtQuick.Window 2.1

QtObject {
    property var mainWindow: Window {
        id: mainWindow
        visible: true
        width: 600
        height: 400
        title: "Hello"
        Column {
            anchors.fill: parent
            Text {
                text: "Hello, world!"
                font.pixelSize: 24
            }
        }
    }
}
