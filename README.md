# Lisp-like language interpreter

Proof of concept to explore low level handling of interpreter and to
revisit my rusty C skills.

- hacky REPL
- no external libraries (just ``libc``)
- single and double quoted ``string`` with some basic escape characters
- ``fixnum`` using immediate value in pointer (``ssize_t`` - 2 bits)
- variety of basic functions inspired by Common Lisp specification
  - ``+``, ``-``, ``*``, ``/``, ``mod``, ``=``
  - ``not``
  - ``list``, ``length``, ``car``, ``cdr``, ``cons``, ``last``
  - ``defvar``, ``setq``, ``let``, ``boundp``
  - ``lambda``
  - ``defun``, ``function`` - global scope
  - ``mapcar``, ``reduce``
  - ``funcall``, ``apply``,
  - ``if``, ``progn``
  - ``puts``, ``print``, ``exit``
  - ``backtrace``
  - and maybe more
- separate function and variable namespace (Lisp-2)

## Quirks

- not a single free since it is not needed for proof of concept
  (reference counting with garbage collector would be next)
- had some issues with optimization at first but later it didn't cause
  issues, actually compiling with ``-O3`` not to make pointer tagging
  work (functions aligned to at least 4 bytes)
- no negative numbers, have to use ``(- number)``
- unrecognized numbers are silently parsed as zero
- no ``float``
- ``lambda`` only binds argument list variables, does not create closure
