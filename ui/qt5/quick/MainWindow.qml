/* Main Window */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0

ApplicationWindow
{
    title: "MettaNode"
    width: 600
    height: 400

    MenuBar {
        Menu {
            title: "Friends"
            MenuItem { text: "Message"; shortcut: "Ctrl+M" }
            MenuItem { text: "Talk"; shortcut: "Ctrl+T" }
            MenuSeparator {}
            MenuItem { text: "Find Friends"; shortcut: "Ctrl+F" }
            MenuItem { text: "Rename"; shortcut: "Ctrl+R" }
            MenuItem { text: "Delete" }
        }
        Menu {
            title: "Window"
            MenuItem { text: "Friends" }
            MenuItem { text: "Search" }
            MenuItem { text: "Downloads" }
            MenuItem { text: "Settings" }
            MenuItem { text: "My Profile"; shortcut: "Ctrl+P" }
            MenuSeparator {}
            MenuItem { text: "Log Window" }
            MenuSeparator {}
            MenuItem { text: "Open synced files" }
        }
    }
    /*StatusBar {
        anchors.bottom: parent.bottom
    }*/

    ColumnLayout {
        anchors.fill: parent
        ListView {
            anchors.fill: parent
            model: ContactModel {}
            delegate: UserCard {}
            highlight: Rectangle { color: "lightsteelblue"; radius: 5 }
            focus: true
        }
    }
}
