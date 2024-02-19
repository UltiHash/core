#!/bin/bash

set -e

private_key_file=uh-license-private.key
public_key_file=uh-license-public.key

openssl genpkey -algorithm ed25519 -out $private_key_file
openssl pkey -in $private_key_file -pubout -out $public_key_file
