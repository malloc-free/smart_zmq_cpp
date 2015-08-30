# SmartZMQcpp

Description
-----------

A wrapper for ZMQ that uses smart pointers, adds some exceptions, plus 
other stuff. 

Why???
------

For a start, I wanted to improve my C++ chops. Secondly, I wanted to 
create my own C++ wrapper, as the existing one did not really suit my personal
style.

Dependencies
------------

This currently only requires the zmq library, and of course a c++11 compatible
toolchain. Oh, and mega_tools, which is not currently here, I will chuck this
up on a repo soon :-)

Silly Stuff
-----------

There are a few _silly_ things going on here, but I am currently learning
how to deal with rh references and proper use of copy and move semantics,
so this will improve over time.

Bugs
----

Currently I am missing a way to automatically close messages when ZMQ 
takes possession of them - will come up with a solution shortly. If you call 
close with a message that ZMQ does not own, it appears to crash :-(


