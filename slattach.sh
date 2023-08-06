# Helper script to setup SLIP connection between Linux and ELKS
#
# run "net start slip" on ELKS after running this script as root
#

if [[ $(id -u) != 0 ]];
then
	echo "Please run me as root."
	exit 1
fi

baud=38400
device=/dev/ttyS0
protocol=slip
elks=10.0.2.15
gateway=10.0.2.2

# stop any previously running slip
pkill slattach

# attach slip to serial line
/sbin/slattach -p $protocol -s $baud -v -L $device &

# configure sl0 interface
/sbin/ifconfig sl0 up $gateway pointtopoint $elks netmask 255.255.255.0 mtu 1024

# display connection
/sbin/ifconfig
/sbin/route
