cc elks-compress.c -o elks-compress
./elks-compress ls.bin ls.z
cp ls.z /Users/greg/net/elks-gh/elkscmd/rootfs_template/root
./elks-compress sh.bin sh.z
cp sh.z /Users/greg/net/elks-gh/elkscmd/rootfs_template/root
./elks-compress nano.bin nano.z
cp nano.z /Users/greg/net/elks-gh/elkscmd/rootfs_template/root
