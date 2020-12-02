# Team MIT HTTP Server Project
### Note: To run this server, you must have CMake Version 3.18 installed on your computer.
## Basic Information
Program written in: C
Technology used: Sockets, pthreads, NCURSES GUI, subprocesses
Supports: Linux, FreeBSD and macOS
HTTP Methods Supported in this Server: GET, POST, HEAD
## Set-Up
First, git clone this repo to a preferred folder. Then, in the Terminal, locate the folder where you cloned this project
to, and change your directory. Type ```cmake .``` and hit enter. Then, type ```cmake --build .``` and hit enter. This will assemble
the program with GUI.
## Run with GUI
To run the server, type ```./server -gui``` to get the GUI where you can edit the port number, choose threads or processes, change the index file and change the error file. After you have made the change, hit F4 to save the change (write to the config file) and then hit F2 to quit. Now you have started your server!
## Run with CMD Line
To run the server this way, you can also type ```./server -p 8000``` or whatever port number you wish to run the server on.
Hit enter and the server will also be started. You can change other configs in the config file.
## Run with Default Settings
If you just type in ```./server ```in the Terminal, you will be able to run the server with the default settings. Please refer to the next section for default settings.
## Default Settings
Port Number: **49157**  
Threads (T) or Processes (P): **T**  
Index page: **index.html**  
404 Not Found Page: **404.html**  
## Features
#### GET
Our server is able to handle GET requests
Run ```http://localhost:49157/tester.html```.  
***OR***  
Open a browser and type ```http://localhost:49157```. 

#### HEAD
Our server is able to handle HEAD requests
In another terminal, type ```curl -I http://localhost:49157/tester.html``` and run the curl command.

#### POST
Our server is able to handle POST requests
In another terminal, type ```wget --post-data "param1=value1&param2=value2 " http://localhost:49157/tester.html```.
This sends the data to the server, and is stored in the folder.  
***OR***  
In another Terminal, type ```curl --data "param1=value1&param2=value2" http://localhost:49157/tester.html``` and run the curl command.
Curl doesn’t store, but shows that the data has been transferred from client to server and is read.
## Other Note(s)
The client.c is just to demonstrate that multiple clients can connect to server at the same time. It cannot make HTTP requests. To assemble this file, run ```gcc client.c -o client``` and run ```./client```. 
