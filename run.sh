#!/bin/bash

# Check if the port number is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port_number>"
    exit 1
fi

# Port number provided as argument
port=$1

# Start the server with the provided port number in the background
./bin/server $port &

# Capture the process ID of the server
server_pid=$!

# Run the client with the same port number
./bin/client

# After the client is done, kill the server
kill $server_pid
