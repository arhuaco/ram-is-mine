ram-is-mine
===========

Ensure that programs do not use more RAM memory than they are allowed to.

Some programs have their own malloc implementations and they do not return RAM to the OS.

I wanted to try this way. I could try to use ulimit and run the process with its own
user, but I think it will be handy to have this program around.

It will add a bit of overhead for small allocations. This will be optimized
later.

Usage
===========

MY\_RAM\_LIMIT=$((1024\*1024\*1024\*2))  LD\_PRELOAD\_64=./ram-is-mine.so LD\_PRELOAD=./ram-is-mine.so test\_1

Replace test\_1 with your program.

Status
===========

Unstable. It will crash your computer and kill the cat.

It is not released yet, it should be completed soon.

Build UNIX
===========

Requirements:
===========

Tested in Ubuntu 14.04.

* glibc
  * We rely on glibc symbol _\_libc\_calloc (and friends) being present. Used during initialization.

* uthash
  * Ubuntu: uthash-dev
  * Project: http://troydhanson.github.io/uthash/.

Credits
===========

* This S.O. answer got me started.
  * http://stackoverflow.com/questions/6083337/overriding-malloc-using-the-ld-preload-mechanism
