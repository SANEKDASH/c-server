# C HTTP server
This project is a HTTP server implemented on C language.

It allows you to browse files located on the host machine.

## Instalation 
- ssh:
```bash
  git clone git@github.com:SANEKDASH/c-server.git
```
- https:
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
## Usage
After you started hosting the server you can connect to it via browser.

Go to the following address in your browser:
```
  http://127.0.0.1:8080
``` 
If there is everything OK with the connection you must be able to see your directory on the browser screen:
![alt text](img/serv.png) 