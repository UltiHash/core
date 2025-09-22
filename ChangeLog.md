## [1.7.0] 2025-09-22
- Include proxy service
- Add missing hash calculation for large objects

## [1.6.1] 2025-09-16
- Fix an error in database migration

## [1.6.0] 2025-07-16
- Add support for S3-compatible object versioning
- Fixes an issue when operating erasure-coded storage groups with disabled
  deduplication
- Fixes an issue where storage service instances might wrongfully acknowledge
  write operations of data that has not been flushed to persistent storage yet

## [1.5.0] 2025-07-08
- Fix issue with get-object which lead to stale downloads with high CPU utilization
- Improved write performance of erasure-coded storage groups
- Enabled reference-count based deletion support for erasure-coded storage groups

## [1.4.0] 2025-06-04
- Add missing Last-Modified header to GetObject
- Bump CLI11 version to 2.5.0
- Bump magic-enum to 0.9.7
- Bump nlohmann-json to 3.12.0
- Bump zppbits to 4.5
- Bump etcd-cpp-apiv3 version to 0.15.4
- Bump lmdbxx to latest version
- Bump cpr to version 1.11.2
- Bump opentelemetry-cpp to 1.20.0
- Add support for erasure-coded storage groups

## [1.3.1] 2025-04-04
- Add labels to distinguish users on metrics

## [1.3.0] 2025-02-24

- Implement support for pay-as-you-go licensing
- Add API call to query configured UH cluster license
- Add support for AWS signature version 4 pre-signed URL
- Add support for per-bucket CORS configuration using `s3:PutBucketCors`,
  `s3:GetBucketCors` and `s3:DeleteBucketCors`
- Set HTTP status messages to better mimic S3 specification
- Allow bypassing deduplication step on data upload
- Introduce coordinator service

## [1.2.1] 2025-02-21

- Fixed an issue in selecting the correct action when passing a query string.

## [1.2.0] 2025-01-21

- Consider configured log level in OTEL logging
- Support generation of OTEL traces to improve debugging capabilities
- Fixed a concurrency issue in storage
- Improved cluster startup by polling for etcd connectivity
- Fixed an issue that would prevent completely reclaiming storage of deleted objects

## [1.1.2] 2024-12-19
- Modified `STORAGE_WRITE_REQ` message for more efficient interaction between deduplicator and storage
- Improved efficiency of reference counting infrastructure in storage service
- Fix broken return value in Content-Range for ranged get access

## [1.1.1] 2024-12-09
- Request downstream error to be treated as an internal error to allow the client to retry
- Synchronized access to database directory
- Re-organized build process

## [1.1.0] 2024-11-11
- Re-connect when losing connection to etcd
- Support authentication against etcd
- Correctly report original size in responses to PutObject and CompleteMultipartUpload
- Fix request ID for `100 Continue` responses
- Add support for policy variables: `uh:ActionId`, `uh:ResourceArn`,
  `aws:username`, `aws:userid`, `aws:PrincipalArn`, `aws:SourceIp`, `aws:referer`,
  `aws:UserAgent`, `s3:x-amz-content-sha256`, `s3:x-amz-copy-source`, `s3:delimiter`,
  `s3:prefix`, `aws:CurrentTime`
- Add default content-type for objects
- Fix a bug in parsing HTTP range requests
- Remove output to log.log
- Add support for date condition operators: `DateEquals`, `DateNotEquals`,
  `DateLessThan`, `DateLessThanEquals`, `DateGreaterThan`, `DateGreaterThanEquals`
  - Conditions should be less than `2261-01-01T00:00:00Z`
- Reduced size of object metadata
- Improved read performance
- Fix a bug in storage, leading to failed deduplication requests
- Improved robustness for successive write-delete-write command sequences and small object sizes
- Fix bug on `NotAction`, `NotPrincipal`, `NotResource`: implement NOR for multiple
  context values
- Implement special characters: `${*}`, `${?}`, `${$}`
- Support variable on resource policy statement
- Prevent services from being destroyed while etcd is not booted up
- Restrictions on naming buckets have been resolved (e.g. cannot start with a number)

## [1.0.0] 2024-10-15
- Added basic policy and authentication support
- Finished deletion support

## [0.6.0] 2024-09-18
- The storage service can now perform delete operations and reclaim disk space
- Deduplicator and Entrypoint services are now making use of storage-level delete operations
- Fix potential data corruption issue caused by deleting and re-uploading data
- Partial implementation of erasure coding / storage groups
- Partial implementation of authentication / policy support

## [0.5.4] 2024-08-08
- Support for object mime types
- Improved handling of multipart uploads
- Added support for using multiple working directories/storage devices to storage service
- Added link/unlink interface to storage service, enabling deletion of unused data

## [0.5.3] 2024-07-01
- Several S3A compatibility fixes

## [0.5.2] 2024-06-25
- Increased waiting time for etcd to 5 minutes

## [0.5.1] 2024-06-24
- Improved compatibility level with S3 API
- Fixed handling of XML-reserved characters in object keys
- Added REST API for querying raw and effective size of data in the cluster
- Fixed incorrect XML in S3 API responses
- Fixed occasional application hangs, resulting in bad performance
- Improved deduplication algorithm
- More efficient storage usage by truncating unused spaces in files

## [0.5.0] 2024-06-06
- Removed support for S3 Outpost's GetBucket
- Deduplication statistic collection
- Improve API support for PySpark (copy object, head object, etc.)
- Replace directory service with PostgreSQL
- Stablize memory requirements for deduplication
- Performance improvements for deduplication

## [0.4.2] 2024-05-22
- Fixed two bugs that result in poor deduplication
- Support running as monolith
- A few bug fixes in S3 API
- Performance improvements

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
- Fixed concurrency issue in directory service
- Fix hanging server when requesting empty files
- Use buffered file I/O to improve performance of deduplicator
  WARNING: this breaks compatibility with instances
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
