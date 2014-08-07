ram-is-mine
===========

Ensure that programs do not use more RAM memory than they are allowed to.

Google Chromium is a memory hog and sometimes it uses so much RAM that the
computer starts trashing.

I wanted to try this way. I could use ulimit and run the process with its ouwn
user, but I think it will be handy to have this program around.

It will add a bit of overhead for small allocations. This will be optimized
later.

Usage
===========

LD\_PRELOAD\_64=./ram-is-mine.so LD\_PRELOAD=./ram-is-mine.so ./test\_1

Replace test\_1 with your program.

Status
===========

Unstable. It will crash your computer and kill the cat.

It is not released yet, it should be completed soon.

Build UNIX
===========

Requirements:
===========

* uthash
  * Ubuntu: uthash-dev
  * Project: http://troydhanson.github.io/uthash/.

Credits
===========

This S.O. answer got me started.

* http://stackoverflow.com/questions/6083337/overriding-malloc-using-the-ld-preload-mechanism
