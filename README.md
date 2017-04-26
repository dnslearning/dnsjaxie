
# dnsjaxie

dnsjaxie is a DNS server that intercepts incoming packets and conditionally
serves responses with unqiue IPv6 endpoints designed to drive a user to an
HTTP site.

## Building

Grab code, make code, install code.

     git clone https://github.com/smartmadre/dnsjaxie.git
     cd dnsjaxie
     sudo apt install build-essential g++ libmysqlclient-dev libmysqlcppconn-dev
     make


## Configuration

`dnsjaxie` by default opens a configuration file at `/etc/dnsjaxie.conf` or
whatever file passed by the `-f <file>` option. The file is a simple text-based file with the following options:

```
dbname dnsjaxie
dbhost localhost
dbuser root
dbpass hunter42
dnshost 8.8.8.8
dnsport 53
```
