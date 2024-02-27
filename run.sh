#!/bin/bash

# Check if the port number is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <port_number>"
    exit 1
fi

# Port number provided as argument
port=$1

# Check if port number is provided
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
fi

# Kill the process
echo "Killing process $pid running on port $port"
kill $pid

# Start the server with the provided port number in the background
./bin/server $port &

# Capture the process ID of the server
server_pid=$!

# Run the client with the same port number
./bin/client $port

# After the client is done, kill the server
kill $server_pid
