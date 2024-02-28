FROM python:3.11.8-bookworm

RUN apt-get update && \
        apt-get install -y tox pytest && \
        rm -rf /var/lib/apt/lists/*
