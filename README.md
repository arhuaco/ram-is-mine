ram-is-mine
===========

Ensure that programs do not use more RAM memory than they are allowed to.

Usage
===========

LD_PRELOAD_64=./intersect_malloc.so LD_PRELOAD=./intersect_malloc.so [your program]

Build UNIX
===========

Requirements:
===========

* uthash

Ubuntu: uthash-dev
Project: http://troydhanson.github.io/uthash/.
