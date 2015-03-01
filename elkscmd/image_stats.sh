#!/bin/sh

# Show misc. statistics for ELKS disk images

MTPT=/mnt/elks
IMAGES="root comb360 comb comb_net full5 full3 full1680"

echo
echo "  ELKS disk image file statistics"
echo "-----------------------------------"

printf "|%7s %s %5s|%5s|%5s|%5s|\n" "Image" '|' "Total" "Used" "Free" "%use"
echo '+--------+------+-----+-----+-----+'
umount $MTPT 2>/dev/null
umount $MTPT 2>/dev/null
for X in $IMAGES
	do if [ -e $X ]
		then
		mount $X $MTPT
		INODES="$(find $MTPT | wc -l)"
		TOTAL="$(df $MTPT | grep "$MTPT" | sed 's/  */ /g' | cut -d' ' -f2)"
		USED="$(df $MTPT | grep "$MTPT" | sed 's/  */ /g' | cut -d' ' -f3)"
		FREE="$(df $MTPT | grep "$MTPT" | sed 's/  */ /g' | cut -d' ' -f4)"
		PCT="$(df $MTPT | grep "$MTPT" | sed 's/  */ /g' | cut -d' ' -f5)"
		printf "%8s %s %5s %5s %5s %5s\n" "$X" '|' "$TOTAL" "$USED" "$FREE" "$PCT"
		umount $MTPT
	fi
done

echo
