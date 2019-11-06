# Computer-Networks Project 1

Connor Mclaughlin and David Martindale

Server Usage: `./QRServer -p <port> -m <maxRateRequests> -r <maxRateWindow> -u <maxUsers> -t <timeOut>`
Client Usage: `./Client -f <filename> -p <port> -a <address>`

Current Status by Rubric Task:
1: Basic TCP connection works and code compiles
2. QR functionality in place.
3. Support for concurrent clients via processes.
4. Error checking on filesize and other QR-functions in place.
5. Function for rate limiting working, but currently blocks under some conditions so we have left it commented out -- see the Server::Accept function for details.
6. Dynamic port and command line done with getopt, shorthand usage written above.
7. Time outs supported by setsockopt().
8. Infrustructure in place for maximum users, but due to issues we had with waitpid's non-blocking option, we do not reject clients automatically if there are too many concurrent users. See main function in server.
Thanks!

Statement of authorship - authorship.jpeg