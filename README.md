ram-is-mine
===========

Ensure that programs do not use more RAM memory than they are allowed to.

Google Chromium is a memory hog and sometimes it uses so much RAM that the
computer starts trashing.

There might be other ways to do this, but I wanted to try this way.

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

Ubuntu: uthash-dev
Project: http://troydhanson.github.io/uthash/.
