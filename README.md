# HTTP-Proxy
An HTTP proxy server which recevies http request from client and returns the demanded answer. <br/>
This is a thread-safe project which supports multithreading via thread-pool. <br/>

### Usage
The project have 2 modes, in the first mode server acts as a proxy server, recevies a request, passes it to the origin server, gets the answer from the server and passes it to the client.</br>
In the second mode, server acts as the main server, recevies a request , searches through the server files for the answer and passes the answer to the client.<br/>
```
./httpserver --proxy www.google.com:80 [--port 8000 --num-threads 5]
./httpserver --files files/ [--port 8000 --num-threads 5]
```
Use the first command to enter proxy mode, second command to enter the server mode.
`--files` flag is used to set the server database directory in the server mode.</br>
`--proxy` to enter proxy mode, the origin server comes right after this flag.<br/>
`--num-threads` is for the number of threads in thread-pool.</br>
`--port` flag is the open port number of the server which is defined by the `server_port` variable in `httpserver.c`.<br/>
`localIP` ip address and `localPort` port. you can change these variables to your own server ip, port.
### Build
Use `make` to build the project. The executable file will be in the `dist` directory. <br/>
`make clean` to clean the artifacts. <br/>
