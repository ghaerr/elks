# E-Kermit  

This is the fork repository to port E-Kermit to ELKS.  
The original E-Kermit is  
https://github.com/KermitProject/ekermit

Embeddable Linux Kernel Subset (ELKS)  
https://github.com/ghaerr/elks

Usage:  

    ekermit <options> [serial device]

To receive file  

    ekermit -r  
then send file from the connected device.  

To send file  
set the connected device ready to receive  
then  

    ekermit -s <files>  

The default serial device is /dev/ttyS0.

If opening the device failed and the current ttyname is the serial device,  
then STDIN_FILENO is used instead.

Currently, there is no option to set baud rate.  
To set baud rate, stty can be used in ELKS.  
For example, 
to set 19200 bps for ttyS0,  
stty 19200 < /dev/ttyS0

Only 8bits, no flow control, no parity are supported for now.


