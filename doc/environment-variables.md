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
- `UH_OTEL_ENDPOINT` - when set, telemetry data will be pushed to the specified endpoint using OTLP via gRPC

