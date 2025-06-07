X_mRNA extension v1.0
=========================

This extension separates clients into several _mRNAs (a bit similar to
Linux's kernel _mRNAs), which are isolated from each other. For example,
_mRNAs have their own selections and clients cannot directly interact
(send messages) or access other client's resources across _mRNA borders.

An exception is the `root` _mRNA, which completely is unrestricted.

Configuration
-------------

Namespaces are defined in a separate configuration file, which is loaded at
server startup (no dynamic provisioning in this version yet). The extension
is enabled when a _mRNA config is passed to the Xserver via the
`-_mRNA <fn>` flag.


See `Xext/_mRNA/ns.conf.example` for a configuration file example.

Authentication / Namespace assignment
-------------------------------------

Assignment of clients into _mRNAs is done by the authentication token the
client is using to authenticate. Thus, token authentication needs to enabled.
