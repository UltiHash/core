# Changelog
## [0.5.0] 2024-06-06
- Removed get bucket command
- Deduplication statistic collection
- PySpark fixes (copy object, head object, etc.)
- Removal of cold fragments
- Better caching mechanism for deduplication set log
- Pursuing perfect matches without looking into the set
- Use postgresql database instead of directory service

## [0.4.2] 2024-05-22
- Fixed two bugs that result in poor deduplication
- Interfaces for different services, providing merging ability of services together
- A few bug fixes in entrypoint
- Improved performance by removing extra memory initialisations

## [0.4.1] 2024-04-26
- Fixed a bug where data corruption could occur when using multiple deduplicator instances
- Implemented asynchronous write support in storage service
- Improved write throughput of storage service
- Improved handling of command line arguments
- Reduced memory footprint of entrypoint service
- Fixed timeout issues when uploading large objects
- Improved throughput of deduplicator service
- Fixed formatting of timestamps in HTTP response
- Fixed computation of ETAG

## [0.4.0] 2024-03-26
- Refactored data store, mostly lockless writes and completely lockless reads
- Fixed a bug in the free spot manager
- Fixed the shared lock concurrency issue in directory service
- Fix hanging server when requesting empty files
- Use buffered file I/O to improve performance of fragment-set log.
- WARNING: New layout of fragment-set logfile breaks compatibility with old logfile format.
- Caching fragments before writing them to storage

## [0.3.4] - 2024-03-15
- Fixed delete objects raising segmentation fault in entrypoint

## [0.3.3] - 2024-03-15
- Fixed entity too small error in entrypoint

## [0.3.2] - 2024-03-14
- Fixed cache usage
- Object Metadata in bucket list response
- More unit tests and fixes

## [0.3.1] - 2024-03-14
- Implemented pagination for better download behaviour
- Fixed potential locking issue in the worker pool
- Reduced log messages to single line entries
- Fixed computation of space savings for empty objects

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
