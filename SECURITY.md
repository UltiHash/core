# Security policy

## Supported versions

Security fixes are applied to the latest release only. We recommend always running the most recent version of UltiHash Core.

| Version | Supported |
|---|---|
| Latest (v1.4.x) | ✅ |
| Older releases | ❌ |

## Reporting a vulnerability

**Please do not report security vulnerabilities as public GitHub Issues.**

If you discover a security vulnerability in UltiHash Core, please report it privately so we can address it before it is publicly disclosed.

### How to report

Use GitHub's built-in private vulnerability reporting:

1. Go to [github.com/UltiHash/core/security/advisories](https://github.com/UltiHash/core/security/advisories)
2. Click **"Report a vulnerability"**
3. Fill in the details — what you found, how to reproduce it, and the potential impact

Alternatively, send an email to **hello@ultihash.io** with the subject line `[SECURITY] <brief description>`.

### What to include

A good report helps us respond faster. Please include:

- A description of the vulnerability and its potential impact
- Steps to reproduce, or a proof-of-concept if available
- The version of UltiHash Core affected
- Any suggested mitigations, if you have them

### What to expect

- We will keep you informed as we investigate and work on a fix
- We will credit you in the release notes when the fix is published, unless you prefer to remain anonymous
- We ask that you give us reasonable time to address the issue before any public disclosure

## Scope

This policy covers the `core` repository and the `uh-helm` chart. For vulnerabilities in third-party dependencies vendored in `third-party/`, please also report them to the upstream project.

## Thank you

We appreciate the work of security researchers and the broader community in helping keep UltiHash secure.
