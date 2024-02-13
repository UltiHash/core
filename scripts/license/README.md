# Overview

This directory contains scripts around the UH licenses. All scripts require an
installation of openssl on your machine.

To run these scripts, the following tools must be available:
- openssl command line utility
- xxd

# License Signature Key Creation

This script creates the license signature keys that are used to sign and verify
signatures. Keys are stored in the current working directory as
`uh-license-private.key` and `uh-license-public.key`:

```
$> ./mk_keys.sh
```

# License Creation

This script creates a new license file to be used with UH cluster. The script
expects a customer name and a maximum storage size in GB and produces a license
file in the current working directory:

```
$> ./mk_license.sh "UltiHash-Test" 1024
```

# License Inspection

This script checks whether a given license file can be verified:

```
$> ./chck_license.sh "UltiHash-Test-128TB.lic"
```

# Integration of Key in Application

In order to verify licenses with UH, the public key must be added to the
application. This is done via the include under
`ROOT/src/common/license/license-public-key.inc`. The file is generated from
the generated public key by running:

```
$> ./mk_include_file.sh "uh-license-public.key" "../../src/common/license/license-public-key.inc"
```
