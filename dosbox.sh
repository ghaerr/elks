# Helper to run PC98 or IBM PC images in DosBoxX (dosbox-x-sdl2)
#

# PC98 1232k image from './buildimages.sh fast'
#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1232-pc98.img"

# just-built PC98 1232k image
#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1232.img"

#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "imgmount -size 1024,8,2,77 C image/fd1232.img"

# 1440k PC98 image
#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1440-pc98.img"

# 1440k IBM MINIX image
#exec ./dosbox-x-sdl2 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1440.img"

# 2880k IBM MINIX image
exec ./dosbox-x-sdl2 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd2880.img"

# 2880k IBM FAT image
#exec ./dosbox-x-sdl2 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd2880-fat.img"
