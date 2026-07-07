<img style="display: block; margin: auto;" src="images/wayland.png">

# Preface

This document describes the Wayland architecture and Wayland model of operation.
This document is aimed primarily at Wayland developers and those looking to
program with it; it does not cover application development.

There have been many contributors to this document and since this is only the
first edition many errors are expected to be found. We appreciate corrections.

Yours, the Wayland open-source community November 2012

## Protocol Documentation

This document does not describe the semantics of individual messages sent
between compositors and clients. Consult the following documents to learn about
concrete Wayland interfaces, requests, and events.

- [wayland.xml] - The official documentation of the core protocol.
- [wayland-protocols] - Standardized Wayland extension protocols.
- [wayland.app] - A community-maintained website that renders these protocols,
  and many more, as easily accessible HTML pages.

[wayland.xml]: https://gitlab.freedesktop.org/wayland/wayland/-/blob/main/protocol/wayland.xml
[wayland-protocols]: https://gitlab.freedesktop.org/wayland/wayland-protocols
[wayland.app]: https://wayland.app

## About the Book

This book is written in markdown and converted to HTML using
[mdbook](https://rust-lang.github.io/mdBook).

It supports the [CommonMark](https://commonmark.org/) dialect of markdown plus a number of
widely supported extensions:

- `~~strikethrough~~`
- ```markdown
  footnotes[^note]
  
  [^note]: text
  ```
- ```markdown
  | Tables | Header2 |
  |--------|---------|
  | abc    | def     |
  ```
- ```markdown
  - [x] Task lists
  - [ ] Incomplete task
  ```
- ```markdown
  definition lists
    : This is the definition of a
      definition list
  ```
- ```markdown
  > [!NOTE]
  > Admonitions
  ```
  
The full list of extensions is documented
[here](https://rust-lang.github.io/mdBook/format/markdown.html#extensions).

## Copyright

Copyright © 2012 Kristian Høgsberg

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
