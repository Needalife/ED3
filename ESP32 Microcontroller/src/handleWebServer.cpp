#include "handleWebServer.h"
#include "moveRobot.h"
#include "indexPage.h"

WebServer server(80);

void handleMove() {
    if (server.hasArg("direction")) {
        String direction = server.arg("direction");
        moveRobot(direction);
        server.send(200, "text/plain", "Moving: " + direction);
    } else {
        server.send(400, "text/plain", "Bad Request: Missing 'direction' parameter");
    }
}

void handleRoot() {
    String html = getIndexPage();
    server.send(200, "text/html", html);
}
