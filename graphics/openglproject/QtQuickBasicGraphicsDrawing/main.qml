import QtQuick
import GLFWItem 1.0
import QtQuick.Controls

Item {
    width: 800
    height: 800

    GLFWItem {
        id:glfw
        anchors.fill: parent
    }

    Button {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
		text: "Click me"
		onClicked: {
			glfw.changeTrianglePos();
		}   
    }
}
