#!/bin/bash

set -e

uhServerDB --db-root /data --create-new-root --threads 15 --port 12345
