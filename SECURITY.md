# Security Policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 0.2.x   | ✅ active development |
| 0.1.x   | ⚠️ critical fixes only |
| < 0.1   | ❌ no longer supported |

## Reporting a vulnerability

Please **do not** open a public GitHub issue for security vulnerabilities.

Email **security@qplc.dev** (or open a private advisory via GitHub → Security → Advisories → New draft security advisory) with:

- Description of the vulnerability
- Steps to reproduce
- Potential impact
- Suggested fix (if any)

We will acknowledge within **3 business days** and aim to release a patch within **30 days** for critical issues.

## Scope

The QPLC compiler is a developer tool that runs locally and produces output for industrial PLCs. Security issues we care about:

- **Compiler memory safety** — buffer overflows, use-after-free
- **Parser DoS** — pathological inputs that hang or crash
- **Code injection** — ladder XML or SCL output that, if misinterpreted downstream, could affect PLC behavior
- **Supply chain** — compromised dependencies (we currently have no third-party dependencies except .NET 8 + Avalonia)

The QPLC compiler does **not** run on a PLC, does **not** have a network service (except optional `qplc --lsp` and Modbus simulator server which are for local development only), and does **not** execute user code at runtime.

## Out of scope

- Issues in third-party libraries (.NET runtime, Avalonia, NModbus, etc.)
- Misconfiguration of TIA Portal when importing SCL output (please report to Siemens)
