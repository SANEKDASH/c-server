 # C HTTP server
This project is a HTTP server implemented on C language.

It allows you to browse files located on the host machine.

## Installation 
- ssh:
```bash
  git clone git@github.com:SANEKDASH/c-server.git
```
- HTTPS:
```bash
  git clone git@github.com:SANEKDASH/c-server.git
```
Then change current directory to the project directory:
```bash
  cd c-server  
```
After that run build with the following command:  
```bash
  make  
```
Now you can run server on your machine:
```bash
  ./serv
```

There are also some supported run options:
- __-a__ : Specifies the ip address of the server.
To see all the addresses you can run the server on use [ifconfig utility](https://man7.org/linux/man-pages/man8/ifconfig.8.html).
In case of not using this option the server will run on the __127.0.0.1__ ip address.
- __-p__: Specifies the connection port. In case of not using this option the port will be equal to __8080__.

## Usage
After you started hosting the server you can connect to it via browser.

Go to the following address in your browser:
```
  http://127.0.0.1:8080
``` 
If there is everything OK with the connection you must be able to see your directory on the browser screen:
![alt text](img/serv.png)

## Performance research
As you can see in the code there are several operating modes of the server:
- __default__:
Only one single connection can be handled at one time.
- __multithread__:
Several (16) connection can be handled at one time.
- __coroutine__:
Connections are handled with help of [coroutine library written by me](https://github.com/SANEKDASH/my_coro_lib).
