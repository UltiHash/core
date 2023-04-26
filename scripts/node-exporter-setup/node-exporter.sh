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
  echo "Creating directory Downloads... "
  mkdir "/home/$username/Downloads"
fi

cd "/home/$username/Downloads" || exit
if [ ! -d "/home/$username/Downloads/node_exporter" ]; then
  echo "Building Node Exporter... "
  git clone https://github.com/prometheus/node_exporter.git
  cd node_exporter || exit
  make build
fi

# start node-exporter as a service
if systemctl is-active --quiet node_exporter; then
  echo "Node Exporter is already running."
else
  echo "Starting Node Exporter... "
  sudo useradd --no-create-home --shell /bin/false node_exporter || exit
  cd "/home/$username/Downloads/node_exporter" || exit

  sudo cp node_exporter /usr/local/bin || exit
  sudo chown node_exporter:node_exporter /usr/local/bin/node_exporter || exit
  sudo cp ./node_exporter.service /etc/systemd/system/ || exit

  sudo systemctl daemon-reload || exit
  sudo systemctl start node_exporter || exit
  sudo systemctl enable node_exporter || exit
fi
