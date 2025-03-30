# Helper to run PC98 images in DosBoxX (dosbox-x-sdl2)
#

# 1232k image from './buildimages.sh fast'
#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1232-pc98.img"

# just-built 1232k image
exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1232.img"

# 1440k image
#exec ./dosbox-x-sdl2 -set machine=pc98 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1440-pc98.img"

# 1440k IBM image
#exec ./dosbox-x-sdl2 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd1440.img"

# 2880k IBM image
#exec ./dosbox-x-sdl2 -set "dosbox quit warning=false" -fastlaunch -c "boot image/fd2880-fat.img"
