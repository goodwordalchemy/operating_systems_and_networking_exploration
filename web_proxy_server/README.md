# HTTP Proxy Server

Based on [this assignment](http://www.cs.rochester.edu/courses/257/fall2018/projects/p2.html).  

The idea is to make a server that intercepts outbound http requests from a computer or network.  Then it makes a request on behalf of the client.  When it gets a response, it sends that response back to the client.  

### To use:

`make`.  Then run `./server.out sample.config`.  Finally, configure your network so that it forwards http requests through your proxy.  For my mac, I did this by navigating to `System Preferences > Network > Advanced`.  Then I checked the `Web Proxy (HTTP)` box and put my ip address (`127.0.0.1` for localhost) and host (as specified in config) into the form.  After clicking `okay`, and pressing `Accept`, I restarted my browser.  Then I was all set. 

I spent most of my time on the project figuring out how to parse the config file.  The sockets stuff was not that hard to figure out, because I have a nice little library for them that I need to put in a proper package instead of copying around from package to package...

### To Do:

* I ought to be parsing the http headers to determine the length of the response, rather than just guessing that it is 20000 bytes.
