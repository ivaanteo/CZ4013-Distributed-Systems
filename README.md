# CZ4013-Distributed-Systems

Socket programming stuff

### before running:

-   create bin folder
-   (for server), create "ServerDirectory" folder in root directory to simulate server root directory

### compile:

```
g++ client.cpp -o bin/client
g++ server.cpp -o bin/server
```

### compiling with c++11

```
g++ -std=c++11 server.cpp -o bin/safe-server
```

### run:

-   must specify 8080 as port number as of now
-   use different terminals

```
./bin/server 8080
./bin/client
```
