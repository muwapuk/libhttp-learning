#include "server.h"
#include <iostream>

#include <logger.h>

std::string html = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>My First Web Page</title>
</head>
<body>
    <h1>Welcome to My Website</h1>
    <p>This is a simple paragraph of text on a pure HTML page.</p>
    
    <!-- A simple link to another site -->
    <a href="https://www.w3schools.com">Learn more at W3Schools</a>
</body>
</html>
)";


int main(int argc, char **argv)
{
	libhttp::Server srv;

	srv.Get("/k", [](const libhttp::Request &req, libhttp::Response &res) {
        res.reason = "OK";
        res.status_code = 200;
        res.version = "HTTP/1.0";
        res.headers["Connection"] = "close";
        res.body = html;        
	});

	srv.listen("127.0.0.1", 25566);

	return 0;	
}
