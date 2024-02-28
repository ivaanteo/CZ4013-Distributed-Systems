#!/bin/bash

# Check if the ServerDirectory directory exists
if [ ! -d "ServerDirectory" ]; then
    mkdir ServerDirectory
fi

# Check if the port number is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port_number>"
    exit 1
fi

# Port number provided as argument
port=$1

# Find the PID using lsof
pid=$(lsof -ti :$port)

# Check if process exists
if [ -z "$pid" ]; then
    echo "No process found running on port $port"
else
    # Kill the process
    echo "Killing process $pid running on port $port"
    kill $pid
fi

# Start the server with the provided port number in the background
./bin/server $port

# Capture the process ID of the server
server_pid=$!

# kill the server
kill $server_pid
