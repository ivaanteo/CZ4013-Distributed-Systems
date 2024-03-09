# CZ4013-Distributed-Systems

Socket programming stuff

### before compiling:

-   create bin folder

### compile both at once

```
make all
```

### run:

-   use different terminals

```
./runServer.sh <serverPort> <invocation type: 0 for at-least-once, 1 for at-most-once>
./runClient.sh <serverPort> <clientPort>
```