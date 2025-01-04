#include "indexPage.h"

String getIndexPage() {
    return R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Controller</title>
        <style>
            #joystick-container {
                width: 300px;
                height: 300px;
                background-color: #f0f0f0;
                border-radius: 50%;
                position: relative;
                margin: auto;
            }
            #joystick {
                width: 100px;
                height: 100px;
                background-color: #007bff;
                border-radius: 50%;
                position: absolute;
                top: 50%;
                left: 50%;
                transform: translate(-50%, -50%);
            }
            #camera-stream {
                display: block;
                margin: 20px auto;
                border: 2px solid #ccc;
                max-width: 100%;
            }
            h1 {
                text-align: center;
            }
        </style>
    </head>
    <body>
        <h1>Mecanum Wheelchair</h1>
        <img id="camera-stream" src="http://<ESP32-CAM-IP>:81/stream" alt="Camera Stream">
        <div id="joystick-container">
            <div id="joystick"></div>
        </div>
        <script>
            const joystick = document.getElementById("joystick");
            const joystickContainer = document.getElementById("joystick-container");

            let containerRect = joystickContainer.getBoundingClientRect();
            let centerX = containerRect.width / 2;
            let centerY = containerRect.height / 2;

            let isDragging = false;

            joystick.addEventListener("mousedown", startDrag);
            joystick.addEventListener("touchstart", startDrag);

            document.addEventListener("mousemove", drag);
            document.addEventListener("touchmove", drag);

            document.addEventListener("mouseup", endDrag);
            document.addEventListener("touchend", endDrag);

            function startDrag(e) {
                isDragging = true;
            }

            function drag(e) {
                if (!isDragging) return;

                let x, y;
                if (e.touches) {
                    x = e.touches[0].clientX - containerRect.left;
                    y = e.touches[0].clientY - containerRect.top;
                } else {
                    x = e.clientX - containerRect.left;
                    y = e.clientY - containerRect.top;
                }

                const maxDistance = containerRect.width / 2 - joystick.offsetWidth / 2;
                const distance = Math.sqrt((x - centerX) ** 2 + (y - centerY) ** 2);
                if (distance > maxDistance) {
                    const angle = Math.atan2(y - centerY, x - centerX);
                    x = centerX + Math.cos(angle) * maxDistance;
                    y = centerY + Math.sin(angle) * maxDistance;
                }

                joystick.style.left = `${x}px`;
                joystick.style.top = `${y}px`;

                const dx = x - centerX;
                const dy = centerY - y;
                let direction = "stop";
                if (Math.abs(dx) > Math.abs(dy)) {
                    direction = dx > 0 ? "right" : "left";
                } else if (Math.abs(dy) > Math.abs(dx)) {
                    direction = dy > 0 ? "forward" : "backward";
                }

                sendCommand(direction);
            }

            function endDrag() {
                isDragging = false;
                joystick.style.left = "50%";
                joystick.style.top = "50%";
                sendCommand("stop");
            }

            function sendCommand(direction) {
                fetch(`/move?direction=${direction}`)
                    .then(response => response.text())
                    .then(text => console.log("Response: ", text));
            }
        </script>
    </body>
    </html>
    )rawliteral";
}
