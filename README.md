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

On root directory

```./httpd```

For more information see ./httpd.8

## Run manager

``./httpdctl [ip port]``

Ip and port are optionals and it must be both of them or neither of them.

The username is "manager" and the password is "pdc69"

## Command of manager

It support pipe-linning, the syntaxis is:

````

  cmd

  cmd

  .

  .
  
  .
  
  cmd
  
  .
  
  ````
  
Note the it finish with the '.' character and cmd is one of the following commands:

* Gets the transformation command

``get cmd``

* Gets the list of media range

``get mime``

* Gets the estate of the transformations

``get tf``

* Gets the quantity of concurrents connections

``get mtr cn``

* Gets the quantity of historics connections

``get mtr hs``

* Gets the quantity of transfer bytes

``get mtr bt``

* Change the transformation command for th command parameter

``set cmd command``

* Adds the media-range parameter to the list of media-ranges from the proxy

``set mime media-range``

* Resets the list of media-ranges from the proxy

``set mime``

* Turn on or off the tranfomations

``set tf on/off``

* Sends a request Bye to the server

``bye``


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

## Logs

Logs will create a folder logs under the directory that you run the
httpd binary. In this folder it will create a file for each level of
log (access, debug, error).
