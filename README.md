# ChatServer
README
Name: Ben Sagir
Id: 206222200

Exercise 3 - Hypertext Transfer Protocol Server.

=== Description ===
The following program is a HTTP server working on both version 1.0 and 1.1.
there are some kind of request the server know how to handle:
	1. GET request for a file.
	2. GET request for a directory.
    
there are many types of responses the server send to the client:
    1. 200 OK - the server will send the client the asked file or directory index.
    2. 302 Found - the directory asked by the client does not end with slash.
    3. 400 BAD_REQUEST - the first line of the request does not contain path or cretifed protocol.
    4. 403 FORBIDDEN - when the client asks for a file or directory he does not premitted to get.
    5. 404 NOT_FOUND - when the client asks for directory or a file that does not in exist in the server.
    6. 500 INTERNAL_SERVER_ERROR - a response sent because of an error at the server side.
    7. 501 NOT_SUPPORTED - a response for a non GET request. 

Program files:
	server.c - contain the code of the server and the main.
    threadpool.c - contain a library that allows parallel programing.

To compile, open the terminal in the directory and write: "gcc server.c threadpool.c -o server -Wall -lpthread"
In order to execute the server you must have "threadpool.h" in the same directory.
To get response from the server you must send a request for either the browser or the telnet.
Notice that if you are running from the browser, it tend to save the responses given in it's cache what may lead to Segmentaion Fault (Core Dump).
To prevent it from happening, before make a request, clear the cache of the browser.
