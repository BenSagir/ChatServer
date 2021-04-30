README
Name: Ben Sagir
Id: 206222200

Exercise 4 - Select Chat Server.

=== Description ===
The following program is a chat server working on tcp connection and select() system call.
the program allows users to connect to the port given, and broadcast a message from a user to all other connected users.

Program files:
	chatserver.c - contain all the code of the server and the main.

To compile, open the terminal in the directory and write: "gcc chatserver.c -o server -Wall -g"
In order to execute the server you should write in the terminal "./server <port>" where port is a positive number.
To get messages from the other connected users, you can connect with terminal "telnet localhost <port>".
once you are in, all the messages that a user send to the server will broadcast to you as well.

to close the program, press Ctrl+C in the terminal running the server code.
