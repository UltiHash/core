# Changelog

## [0.3.0] - 2024-03-13
- Fixed potential log corruption issue in deduplicator
- Increase maximum storage service size
- Logging of service requests
- Add counter counting active connections

## [0.2.1] - 2024-03-06
- Fix access violations in entrypoint and deduplicator
- Fix memory leak in license validation
- Fix double-free in MD5 checksum computation
- Fix use-after-free in directory bucket deletion
- Finalized multipart upload and pending deletion of multipart uploads
- Propagate errors about missing downstream services in entrypoint

## [0.2.0] - 2024-02-29
- Implement etcd-based service coordination
- Use open-telemetry for metrics and log reporting
- Implement basic licensing
- Fix several stability and performance issues
