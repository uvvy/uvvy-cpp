/* Contact List */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

Window {
    title: "Contacts"
    width: 600
    height: 400

    ColumnLayout {
        anchors.fill: parent

        ListView {
            anchors.fill: parent
            delegate: UserCard {}
            model: ContactModel {}
            focus: true
            clip: true
        }
    }
}
