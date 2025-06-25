# X11Libre Security Policy

##  Reporting Vulnerabilities

We take security seriously in X11Libre. If you discover any vulnerabilities, please report them responsibly.

- **Contact**: https://github.com/metux info@metux.net legendarydood@gmail.com
- **Preferred Method**: Email with detailed reproduction steps, logs, and system info 
- **Public Disclosure**: Please wait until we’ve resolved the issue before making it public 

##  Supported Versions

| Version         | Status                    |
| --------------- | ------------------------- |
| `master` branch |  Supported and maintained |
| Older tags      |  No longer supported      |

We recommend always using the latest release for performance and security fixes.

##  Security Best Practices (User-Side)

To help protect your systems when using x11libre:

- Use minimal privileges when running X sessions 
- Avoid setuid binaries unless required 
- Keep your display manager and window manager updated 
- Regularly audit any X11-forwarded connections, especially over SSH 
- Use sandboxing or containerization when integrating third-party extensions

##  Developer Guidelines

For contributors submitting PRs:

- Don’t introduce new system calls without justification 
- Avoid unsafe memory operations (especially in C/C++) 
- Use compile-time and runtime hardening flags 
- Submit fuzzing harnesses or test vectors for complex parsing logic 

---

We appreciate your help in keeping x11libre safe for everyone. Let’s build something resilient, secure, and libre.
