# Reliable Data Transfer Protocol

This project is my implementation of [this assignment](http://www.cs.rochester.edu/courses/257/fall2018/projects/p3.html) from the University of Rochester.  I was able to implement a one-way reliable data transfer protocol that passed the tests in the assigment description.  Namely, the protocol I implemented can do the following:

* Send multiple packets at once and confirm their correct delivery at the receiver.
* Handle out of order packets.
* Handle messages whose lengths are greater than the maximum packet size for the transport layer.
* Handle multiple incoming messages at once from the upper layer.
* Handle data corruption in the underlying link layer.
* Hanlde lost packets.

I implemented a go-back-n protocol.  I did not buffer on the receiving end, but maybe I'll come back to this later and implement that.

It took me a day and a half to finish this project.  
