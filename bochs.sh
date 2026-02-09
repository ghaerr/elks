# Helper to run ELKS in bochs
#
# Prior to running this script:
# export BXSHARE= to bochs installation directory

[ -z "$BXSHARE" ] && export BXSHARE=/usr/local/Cellar/bochs/3.0/share/bochs

# default image is 2880k floppy, change floppya: in bochs.rc for other sizes
bochs -qf bochs.rc
