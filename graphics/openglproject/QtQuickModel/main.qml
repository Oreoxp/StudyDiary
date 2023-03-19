import QtQuick 2.4
import GLFWItem 1.0
import QtQuick.Controls 2.4
import QtQuick.Window 2.0

Item {
    width: 800
    height: 800
    focus: true

    Timer {
        interval: 16 // 60 fps
        running: true
        repeat: true
        onTriggered: {
            glfw.update(); // force QQuickFramebufferObject to redraw
        }
    }

    GLFWItem {
        id:glfw
        anchors.fill: parent
    }





    
    Button {
        id:translate
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
		text: "Click me"
		onClicked: {
			glfw.changeTrianglePos();
		}   
    }
    Button {
        id:leftbtn
        anchors.bottom: translate.bottom
        anchors.right: translate.left
		text: "left"
		onClicked: {
			glfw.changeKeyDown(GLFWItem.DOWN_LEFT);
		}   
    }
    Button {
        id:rightbtn
        anchors.bottom: translate.bottom
        anchors.left: translate.right
		text: "right"
		onClicked: {
			glfw.changeKeyDown(GLFWItem.DOWN_RIGHT);
		}   
    }
    Button {
        id:upbtn
        anchors.left: translate.left
        anchors.bottom: translate.top
		text: "up"
		onClicked: {
			glfw.changeKeyDown(GLFWItem.DOWN_UP);
		}   
    }
    Button {
        id:downbtn
        anchors.left: translate.left
        anchors.top: translate.bottom
		text: "down"
		onClicked: {
			glfw.changeKeyDown(GLFWItem.DOWN_DOWN);
		}   
    }
    
    Keys.onPressed: {
        if (event.key === Qt.Key_Escape) {
            Qt.quit();
        }else if (event.key === Qt.Key_Left) {
            leftbtn.onClicked();
        }else if (event.key === Qt.Key_Right) {
            rightbtn.onClicked();
        }else if (event.key === Qt.Key_Up) {
            upbtn.onClicked();
        }else if (event.key === Qt.Key_Down) {
            downbtn.onClicked();
        }
    }


    Text {
        text: "FPS: " + Qt.application.fps
        font.pixelSize: 16
        color: "white"
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
    }

    Text {
        text: "mesh subdivision: Value: " + (slider.value / 50.0).toFixed(2)
        font.pixelSize: 16
        color: "white"
        anchors.bottom: slider.top
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
    }
    Slider {
        id: slider
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottomMargin: 10
        anchors.right: leftbtn.left
        from: 0
        to: 100
        value: 50
        onValueChanged: {
            glfw.changeSliderValue(value);
        }
    }
}
