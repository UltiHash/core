# Overview

This document lists of all environment variables read by UltiHash cluster
components and describes their purpose.


# Environment Variables

- `UH_POD_IP` - when set it must contain a valid IPv4 or IPv6 address. The
    address is used to advertise the service in etcd.
- `UH_LOG_LEVEL` - when set it overrides the default log level (INFO) and must
    be one of the following severity levels: DEBUG, INFO, WARN, ERROR, or FATAL.
- `UH_LICENSE` - (required) must be filled with a valid UH license (use sample
    licenses under `ROOT/data/licenses` for testing)
- `UH_OTEL_ENDPOINT` - when set, telemetry data will be pushed to the specified endpoint using OTLP via gRPC (format: "hostname:port")
- `UH_WORKING_DIR` - configures the working directory for different services. In specific case of storage service, when multiple working directories are desired, they can be listed and separated by ":".

# Database Connection

- `UH_DB_HOSTPORT` - configure host and port of the DBMS (format: "hostname:port")
- `UH_DB_DIRECTORY_CONNECTIONS` - number of connections used to connect to directory database
- `UH_DB_MULTIPART_CONNECTIONS` - number of connections used to connect to multipart database
- `UH_DB_USER` - configure user name for connections to the DBMS
- `UH_DB_PASS` - pass the connection password for the user given in `UH_DB_USER`

# ETCD Authentication

- `UH_ETCD_USERNAME` - username for etcd authentication
- `UH_ETCD_PASSWORD` - password for etcd authentication
