# http-proxy

## Prerequisites
LINUX
````
sudo apt-get install make gcc libsctp-dev
````
## Setup
On root folder execute

```make```

The proxy server is the root folder with the name httpd.
The manager client is in the root folder with the name httpdclt.

## Run proxy

```./httpd```

For more information see ./httpd.8

## Run manager

``./httpdctl [ip port]``

Ip and port are optionals and it must be both of them or neither of them

## Documentation

The report and presentation will be located under documentation folder

## Code use from course

The following files has been copied from socks5 course proyect

    smt.c
    smt.h
    buffer.c
    buffer.h
    selector.c
    selector.h
