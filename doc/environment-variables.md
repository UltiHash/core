# Overview

This document lists of all environment variables read by UltiHash cluster
components and describes their purpose.


# Environment Variables

- `UH_POD_IP` - when set it must contain a valid IPv4 or IPv6 address. The address
    is used to advertise the service in etcd.
- `UH_LOG_LEVEL` - when set it overrides the default log level (INFO) and must be one of the following severity levels: DEBUG, INFO, WARN, ERROR, or FATAL.

