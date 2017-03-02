
----------------------------------------------------------------------------------
Instructions how to test ethernet with the new version of ktcp and the ne2k driver
----------------------------------------------------------------------------------

Mark-Francois (MFLD) has written an NE2000 network interface card driver for ELKS. To be
able to make use of that I (Georg Potthast / georgp24) modified the existing ktcp user
mode driver by Harry Kalogirou which supported SLIP only to support the ne2k driver now. 
I added ARP support, a link layer, gateway support via the ne2k NIC and fixed 
ntohl() / htonl() in linuxmt/arpa/inet.h. Plus several small changes here and there.

As a result you can now use the ne2k driver for ethernet communication.


----------------------
Loading ne2k and ktcp
----------------------

To load the ne2k driver you have to enter "make menuconfig" in the elks/elks directory.
Then enable "NE2K device driver" in "Driver support -> Network device drivers". When
you then compile ELKS again, the ne2k driver will be loaded.

The ktcp driver, which implements the tcp/ip stack, is a user mode driver. This is 
loaded with the /etc/rc.d/rc.sysinit script by ELKS at during boot.

When you enter "ps" you will see the ktcp process. You can enter "kill 5" to remove it 
and enter "ktcp 10.0.2.15 /dev/eth &" on the command line, it will be loaded again. 
Please do not forget the "&" or you will be locked out.

If you want to use slip instead you have to use an ip address that fits to the network 
on the host and the serial interface, e.g. "ktcp 192.168.1.10 /dev/ttyS0 &". On the 
host you have to use "slattach" and enable a point-to-point connection. Details will 
be available in a different document.


--------------------
Using urlget on ELKS
--------------------

First edit the qemu.sh file in the elks directory to:

# no forwarding, just ELKS to host
hostfwd="-net user"

I found urlget will not work, if the "hostfwd" parameter is specified as well.

Now you can download a web page from the internet with urlget on ELKS. However, since
there is no name resolution yet, you first need the ip address of the web server.

For this I use "ping" on the host. When you enter "ping www.google.com" the ip address
of the server will be displayed. 

You can also use "nslookup" which will display the ip address as "Non-authoritative answer".

Another utility is "dig" which will display the ip address in the "Answer Section".

Anyway, for "www.google.de" I retrieved 216.58.209.67. On ELKS you enter:

"urlget http://216.58.209.67" and the web page will be displayed on stdout. It still 
contains all tags and is difficult to read.


-----------------------------------------
Retrieving web pages on ELKS using httpd
-----------------------------------------

First edit the qemu.sh file in the elks directory to:

# telnet and http forwarding, call ELKS web server with 'http://localhost:8080'
hostfwd="-net user,hostfwd=tcp:127.0.0.1:2323-10.0.2.15:23,hostfwd=tcp::8080-:80"

Comment out the previous specification: #hostfwd="-net user"

As already mentioned, you need this hostfwd parameter since Qemu features a firewall 
which prohibits to access the ip addresses on ELKS. The hostfw parameter enables this 
for the specified address and port.

With the rc.sysinit script, which ELKS runs at boot time, the httpd web server has been
loaded already. Enter "ps" to check this.

This web server is installed with a web page in /usr/lib/httpd. You can read the web page 
if you enter "http://localhost:8080" in the address bar of your browser on the host.

If you want to add further pages you can put these into the "/usr/lib/httpd" directory.
You may e.g. edit the "install:" target in the "elkscmd/inet/httpd/Makefile" to copy these
files into the "/usr/lib/httpd" directory. After compiling and starting ELKS again you can 
then open these with e.g. "http://localhost:8080/mywebpage.html" in your browser.

You can also retrieve simple text files too. When logged into ELKS, if you enter 
"ls -l /bin >test.txt" in the "usr/lib/httpd" directory you can retrieve this file with 
"http://localhost:8080/text.txt" in your browser.


--------------------------------------------
Using the echo server and client for testing
--------------------------------------------

You can also use an echo server on ELKS and access that with an echo client on the host
or vice versa for testing. These programs are in the elkscmd/test/socket/echo directory. 
Use the echoserver_in.c and echoclient_in.c code. 

The other versions are for unix domain sockets on ELKS. If you enable unix domain sockets 
plus the ne2k driver the kernel will exceed 64k and cannot be compiled.

First copy echoclient_in to the host and compile it with gcc. Then start the server
with "echoserver_in &" on ELKS. Check with "ps" it has been loaded successfully. If you 
then run echoclient_in on the host, you can enter something on the keyboard and that will 
be returned. Terminate the echoserver again with the kill command or loading Qemu again 
with new parameters may fail.

Then change the qemu.sh file in the elks directory again to:

# no forwarding, just ELKS to host
hostfwd="-net user"

Then copy echoserver_in to the host and compile it with gcc. Then start it with
"./echoserver_in &" on the host. Run "qemu.sh" and enter "echoclient_in" in ELKS.
Again you can type anything on the keyboard and that will be returned by the server
on the host.


26th February 2017 Georg Potthast
