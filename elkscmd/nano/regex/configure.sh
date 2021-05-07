# Port of libregex to ELKS by Greg Haerr Mar 2021
# from https://github.com/linusyang92/libregex

cc -c -I. -DGAWK -DNO_MBSUPPORT -O2 -std=gnu99 -Wall -Werror regex.c
