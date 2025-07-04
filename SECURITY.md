# X11Libre Security Policy

##  Reporting Vulnerabilities

The X11Libre project takes security seriously. If you discover any vulnerabilities, please report them responsibly.

### How to Report a Security Vulnerability

Send a detailed email to one or more of the following contacts:
- info@metux.net
- legendarydood@gmail.com

Include the following information:

1. **Vulnerability description**
   - What did you observe, and why is it a concern?

2. **Reproduction steps**
   - Clear, step-by-step instructions
   - Include specific configurations or inputs required

3. **System and environment details**
   - OS version
   - X11Libre version or commit hash
   - Display manager, drivers, or hardware specifics

4. **Supporting data**
   - Logs (in plain text)
   - Core dumps (if available and safe to share)

5. **Impact analysis (if known)**
   - Potential for remote or local exploitation
   - Possible consequences (e.g. data exposure, privilege escalation, denial-of-service)

Please allow us ample time to validate and patch the issue before disclosing it publicly.

Feel free to privately message staff over our offical Matrix or Telegram if the issue is of extreme merit and needs an immediate solution. 

##  Supported Versions

| Version         | Status                    |
| --------------- | ------------------------- |
| `master` branch |  Supported and maintained |
| Older tags      |  No longer supported      |

We recommend always using the latest release for performance and security fixes.

##  Security Best Practices (User-Side)

To help protect your systems when using X11Libre:

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

We appreciate your help in keeping X11Libre safe for everyone. Let’s build something resilient, secure, and libre.
