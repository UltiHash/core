#!/bin/bash

username=$(whoami)

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