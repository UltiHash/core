<div align="center">

<img src="https://cdn.prod.website-files.com/63d6aca49010ac629c2c5622/653ba1d36b1028818bf9d2a9_Asset%206.svg" alt="UltiHash" height="60" />

# UltiHash Core

**High-performance, S3-compatible object storage with built-in binary-level deduplication.**  
Built for AI, ML, and data-intensive workloads. Kubernetes-native. Apache 2.0.

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![Release](https://img.shields.io/github/v/release/UltiHash/core)](https://github.com/UltiHash/core/releases)
[![Build](https://img.shields.io/github/actions/workflow/status/UltiHash/core/build.yml?branch=master)](https://github.com/UltiHash/core/actions)
[![Discord](https://img.shields.io/badge/community-Discord-5865F2)](https://ultihash.io/community)
[![Docs](https://img.shields.io/badge/docs-docs.ultihash.io-informational)](https://docs.ultihash.io)

</div>

---

## What is UltiHash?

UltiHash is an object storage system that stores data the way it actually looks — not the way it arrived. It splits every object into variable-size byte fragments and stores each unique fragment exactly once, across all objects, across all formats, continuously. The result is a fully S3-compatible storage cluster that uses significantly less disk space without any changes to your application.

**Key differentiator:** Unlike filesystem-level or object-level deduplication, UltiHash operates at the binary level — it finds and eliminates redundant byte sequences across files of completely different types, names, and sizes. A DICOM scan that shares raw pixel data with an earlier scan? Only the difference is stored.

### Measured space savings on real datasets

| Dataset | Size | Space saved |
|---|---|---|
| DICOM brain MRI scans | 1.51 GB | **67%** |
| JPGs of driving scenarios | 2.41 GB | **52%** |
| PNGs of synthetic textures | 5.89 GB | **50%** |
| WAV speech audio | 0.33 GB | **50%** |
| TIFF climate data | 16.28 GB | **46%** |
| LiDAR driving scenarios | 33.26 GB | **18%** |

> Average savings across typical AI datasets: **up to 60%**. See the [full benchmark table →](https://docs.ultihash.io/operations/save-space-with-deduplication)

---

## Features

- **Binary-level deduplication** — fragment-level storage across all objects and formats, continuously and transparently
- **S3-compatible API** — drop-in replacement, works with any existing S3 client, SDK, or tool
- **Kubernetes-native** — deployed via Helm, runs on any K8s environment, on-prem or cloud
- **Erasure coding** — Reed-Solomon parity-based storage for configurable data resilience
- **Object versioning** — native versioning with object locking
- **IAM + access policies** — user management, policy-based access control
- **Encryption** — in-flight (TLS) and at-rest encryption
- **Fast deletion** — lightweight deletion without full cluster scans
- **Monitoring** — Prometheus-compatible metrics, including deduplication efficiency

---

## Quick start

The fastest way to see UltiHash running is a one-liner that spins up a local test cluster with Docker:

```bash
curl -fsSL https://ultihash.io/1line | bash
```

This sets up a local Docker cluster with a bundled demo license, lets you point it at any directory on your machine, and shows write/read throughput and deduplication savings in your terminal. **No account needed. Nothing is stored outside your machine.** The cluster is torn down automatically when the test completes.

**Prerequisites:** Docker and Python 3 must be installed and running.

[Inspect the script on GitHub →](https://github.com/UltiHash/test-installation/blob/main/quicktest.sh)

---

## Deploy with Kubernetes (Self-Hosted)

UltiHash is deployed via Helm. The Helm chart lives in [UltiHash/uh-helm](https://github.com/UltiHash/uh-helm).

> **ℹ️ License note:** UltiHash Core is in the process of moving to a fully license-free open-source model. In the meantime, use the public demo credentials below — they work for any self-hosted deployment.

**Prerequisites**

- Kubernetes 1.21+
- Helm 3.x
- `kubectl` configured for your cluster

**Authenticate with the UltiHash registry**

```bash
echo "M_X!DFlE@jf1:Ztl" | docker login registry.ultihash.io \
  -u demo --password-stdin
```

**Install**

```bash
helm install my-cluster oci://registry.ultihash.io/stable/ultihash-cluster \
  --namespace ultihash \
  --create-namespace \
  --set license.key="demo:1024:ZZaWuJY0i8v9D3+jRbCwsqmYsUVXQoewklUB5Xju86qzPzsiO9N4Gn67F7a4UayHmOCDjQwe5r+pn/p26a2CCA=="
```

**Verify**

```bash
kubectl get pods -n ultihash
```

Once all pods are running, configure your S3 client to point at the cluster endpoint and use it like any S3-compatible storage.

For full installation guides (on-prem, AWS, GCP), see the [documentation →](https://docs.ultihash.io/installation)

---

## Connect with your S3 client

UltiHash speaks S3. Any existing S3 client works without modification.

**AWS CLI**

```bash
aws s3 mb s3://my-bucket \
  --endpoint-url http://<ULTIHASH_ENDPOINT>

aws s3 cp my-dataset/ s3://my-bucket/ \
  --recursive \
  --endpoint-url http://<ULTIHASH_ENDPOINT>
```

**Python (boto3)**

```python
import boto3

s3 = boto3.client(
    "s3",
    endpoint_url="http://<ULTIHASH_ENDPOINT>",
    aws_access_key_id="<ACCESS_KEY>",
    aws_secret_access_key="<SECRET_KEY>",
)

s3.upload_file("my-file.parquet", "my-bucket", "my-file.parquet")
```

---

## Check your deduplication savings

UltiHash extends the S3 API with a custom endpoint to query effective storage size after deduplication:

```bash
python get_effective_size.py --url http://<ULTIHASH_ENDPOINT>
```

The script is in [UltiHash/scripts →](https://github.com/UltiHash/scripts/tree/main/boto3/ultihash_info)

---

## Integrations

UltiHash works out of the box with the broader data ecosystem:

| Category | Integrations |
|---|---|
| Query engines | Apache Trino, Apache Presto, Apache Spark / PySpark |
| Workflow orchestration | Apache Airflow, AWS Glue |
| Table formats | Apache Iceberg, Icechunk |
| ML frameworks | PyTorch |
| Annotation & labelling | SuperAnnotate |
| Vector databases | Zilliz / Milvus, Neo4j |
| Messaging | Apache Kafka |

Integration guides and code samples: [docs.ultihash.io/operations/prebuilt-connections →](https://docs.ultihash.io/operations/prebuilt-connections)  
Additional scripts and examples: [UltiHash/scripts →](https://github.com/UltiHash/scripts)

---

## Architecture overview

UltiHash is a distributed system composed of the following components:

```
┌──────────────────────────────────────────────────────────┐
│                     UltiHash Cluster                     │
│                                                          │
│  ┌─────────────┐   ┌──────────────┐   ┌──────────────┐  │
│  │  S3 Gateway │   │ Deduplicator │   │   Storage    │  │
│  │  (API layer)│──▶│  (fragment   │──▶│   Nodes      │  │
│  │             │   │   engine)    │   │              │  │
│  └─────────────┘   └──────────────┘   └──────────────┘  │
│          │                                    │          │
│  ┌───────▼───────────────────────────────────▼────────┐  │
│  │              etcd (cluster metadata)               │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
         ▲
         │  S3-compatible API
         │
  any S3 client / SDK / tool
```

The deduplicator runs as a separate service and can be disabled for write-heavy workloads where throughput takes priority over storage savings:

```bash
helm upgrade my-cluster oci://registry.ultihash.io/stable/ultihash-cluster \
  --set deduplicator.enabled=false
```

---

## Building from source

### Prerequisites

- C++20-capable compiler (GCC 12+ or Clang 16+)
- CMake 3.22+
- Docker (for containerised builds)
- Python 3.8+ (for tooling scripts)

### Clone and initialise submodules

```bash
git clone https://github.com/UltiHash/core.git
cd core
git submodule update --init --recursive
```

### Build with CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Build the Docker image

```bash
docker build -t ultihash-core:local .
```

### Run the tests

```bash
cd build && ctest --output-on-failure
```

Pre-commit hooks are configured in `.pre-commit-config.yaml`. Install them with:

```bash
pip install pre-commit
pre-commit install
```

---

## Repository structure

```
core/
├── src/            # Core C++ source (storage engine, API, deduplicator)
├── test/           # Unit tests
├── testing/        # Integration test helpers
├── scripts/        # Build and maintenance scripts
├── doc/            # Internal design documents
├── cmake/          # CMake modules
├── data/           # Test data and fixtures
├── third-party/    # Vendored dependencies
├── tools/          # Developer tooling
├── Dockerfile      # Container image definition
└── CMakeLists.txt  # Top-level build configuration
```

---

## Contributing

We welcome contributions — bug reports, feature requests, documentation improvements, and code.

Before submitting a pull request, please:

1. Read the [Contributor License Agreement](CONTRIBUTOR_LICENSE_AGREEMENT.md) — all contributors must agree to it.
2. Open an issue first for significant changes, so we can align on approach.
3. Follow the existing code style (enforced by `.clang-format` and `.pre-commit-config.yaml`).
4. Write or update tests for any changed behaviour.

For questions, join our [Discord community →](https://ultihash.io/community)

---

## Documentation

Full documentation is at **[docs.ultihash.io](https://docs.ultihash.io)**

Quick links:

- [Test with Docker](https://docs.ultihash.io/installation/test-with-docker)
- [Install Self-Hosted on-premises](https://docs.ultihash.io/installation/install-self-hosted-on-premises)
- [Install Self-Hosted on AWS](https://docs.ultihash.io/installation/install-self-hosted-on-aws)
- [S3-compatible API reference](https://docs.ultihash.io/operations/s3-compatible-api)
- [Deduplication guide](https://docs.ultihash.io/operations/save-space-with-deduplication)
- [Erasure coding](https://docs.ultihash.io/administration/erasure-coding-for-data-resiliency)
- [Encryption](https://docs.ultihash.io/administration/set-up-encryption)
- [Changelog](https://docs.ultihash.io/reference/changelog)

---

## Releases

UltiHash uses semantic versioning. See [Releases →](https://github.com/UltiHash/core/releases) for changelogs.

The latest release is **v1.4.0**.

---

## Support

| Channel | Link |
|---|---|
| Documentation | [docs.ultihash.io](https://docs.ultihash.io) |
| Community (Discord) | [ultihash.io/community](https://ultihash.io/community) |
| Bug reports | [GitHub Issues](https://github.com/UltiHash/core/issues) |

---

## License

UltiHash Core is released under the [Apache License 2.0](LICENSE).

Contributing to this project requires agreeing to the [Contributor License Agreement](CONTRIBUTOR_LICENSE_AGREEMENT.md).

---

<div align="center">
  Made in Berlin · <a href="https://ultihash.io">ultihash.io</a>
</div>
