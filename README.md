# re - A regex package for Stanza

**re** (short for regex) is a package for the [Stanza](http://lbstanza.org) programming language that adds regex features. It depends on the [PCRE](http://www.pcre.org/) library.

## Instructions

- To use the package you must first install PCRE. On Macs, you can use [Homebew](http://brew.sh) or [MacPorts](https://www.macports.org), and on Linux (if your distro doesn't have it already) you can use your native package manager.

- After installing the library, you can use something like:
```
stanza myfile.stanza re.stanza -ccfiles pcre-stanza.c -ccflags "-L/usr/local/include -lpcre" - o output
```
to compile your file that contains an `import re` statement.

## TODO

- Add method regex-replace
- Organize the code by cleaning up unnecessary function calls and other things
- Look into known bugs