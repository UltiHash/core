# Overview

This document describes the format of the UH license string.


# Format Description

A UH license string consists of two fields: the license information and a
digital signature based on ED25519. Both fields are separated by a colon:

```
uh-license-format ::= uh-license-data ':' uh-license-signature
```

License data string is a colon-separated string of following fields:
- customer id
- maximum data referenced by directory service in GB

```
uh-license-data ::= customer-id ':' directory-data-gb
customer-id ::= STRING
directory-data-gb ::= NUMBER
```

Signature data is stored as a BASE-64 encoded binary buffer of the created
signature. The signature is generated from the license data by running ED25519
algorithm using a signature private key. The public key used for verification
is embedded in the application.

```
uh-license-signature ::= base_64(ed25519-signature)
ed25519-signature ::= BINARY
```
