/* Contact List */
import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import QtQuick.Window 2.1

Window {
    title: "Contacts"
    width: 300
    height: 600

        Column {
            anchors.fill: parent

            ListView {
                anchors.fill: parent
                delegate: UserInfoDelegate {}
                model: contactListModel
                focus: true
                clip: true
            }
        }
}
