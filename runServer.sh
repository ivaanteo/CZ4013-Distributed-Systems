#!/bin/bash

# Check if the ServerDirectory directory exists
if [ ! -d "ServerDirectory" ]; then
    mkdir ServerDirectory
fi

# Check if the port number is provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <port_number> <invocation type: 0 for at-least-once, 1 for at-most-once>"
    exit 1
fi

# Port number provided as argument
port=$1
invocation_type=$2

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
./bin/server $port $invocation_type

# Capture the process ID of the server
server_pid=$!

# kill the server
kill $server_pid
