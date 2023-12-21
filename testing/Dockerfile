FROM debian:latest

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    lsof \
    libssl3 \
    prometheus-cpp-dev \
    libpugixml-dev \
    libgrpc++ \
    libprotobuf32 \
# Note: libcpprest depends on a different version of Boost and should be used with care
    libcpprest2.10 \
    && apt-get clean && apt-get autoclean && apt-get autoremove \
    && rm -rf /var/lib/apt/lists/*
