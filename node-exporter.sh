#!/bin/bash

username=$(whoami)

# go
if ! command -v go &> /dev/null
then
    echo "Go is not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y golang
else
    echo "Go is already installed."
fi

# node-exporter
if [ ! -d "/home/$username/Downloads" ]; then
  mkdir "/home/$username/Downloads"
fi

cd "/home/$username/Downloads" || exit
if [ ! -d "/home/$username/Downloads/node_exporter" ]; then
  git clone https://github.com/prometheus/node_exporter.git
  cd node_exporter || exit
  make build
fi

cd "/home/$username/Downloads/node_exporter" || exit
./node_exporter