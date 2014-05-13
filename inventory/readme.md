Monitoring System

The monitoring systems comprises several elements:

* a database (an instance of libojrdf) that stores all the system information (sites, sets, equipment, contacts,..) and the monitoring (logs and reports) 
* a VPN server that accepts connection from remote kiosks, it notifies the database when a kiosk comes on-line and when it leaves, it also provides a list of currently on-line kiosks
* a web server, it is mainly used for access control (basic authentification), all authorized requests are proxied to the database built-in web server

There are 2 users dedicated to the monitoring tasks:

* monitor: runs monitoring jobs on remote hosts and sends the result to the database, it logs in as unicef_admin on the remote machines using public key authentication (older systems do not have the 'unicef_admin' user, it can be added with ```useradd -n -s /bin/bash -G admin,dialout unicef_admin```, alternatively .ssh/config can be modified to use a different user depending on IP address)
* objrdf: runs the database and web server on non-privileged port (1080)


Identifying resources
Natural name for equipment is the serial number (with some restrictions imposed by the RDF standard, eg.: can not start with a digit), 

