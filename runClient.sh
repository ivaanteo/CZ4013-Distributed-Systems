#!/bin/bash

# Check if the port number is provided
if [ $# -ne 3 ]; then
    echo "Usage: $0 <server_ip_address> <server_port_number> <client_port_number>"
    exit 1
fi

# Port number provided as argument
server_ip_address=$1
server_port=$2
client_port=$3

# Find the PID using lsof
pid=$(lsof -ti :$client_port)

# Check if process exists
if [ -z "$pid" ]; then
    echo "No process found running on port $client_port"
else
    # Kill the process
    echo "Killing process $pid running on port $client_port"
    kill $pid
fi

# Start the server with the provided port numbers
./bin/client $server_ip_address $server_port $client_port

# Capture the process ID of the server
client_pid=$!

# kill the client
kill $client_pid
