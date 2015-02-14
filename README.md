# multi-screensaver

X11 Port of the [To Heart Multi screensaver from ELEMENTAL SOFT](http://a.pomf.se/vbmbsb.zip)

![multi2](https://cloud.githubusercontent.com/assets/5257054/6196983/f5c1cfe6-b3de-11e4-86f8-4ef84ac3f8de.gif)

## Requirements
 - libx11-dev
 - libxpm-dev
 Optional
 - xscreensaver (MATE/KDE/Gnome-screensaver will probably run it fine, but on a black background)
 - xwininfo from the x11-utils package(See standalone usage below)
 
## Building
 Run ```make```

## Installation on a system with xscreensaver (XFCE4, lxde, ...)
 - Copy the executable "multi" to somewhere in you $PATH, ie: ```sudo cp multi /usr/local/bin/hmx12multi```
 - Overwrite/create the configuration file with ```echo 'programs: hmx12multi'~/.xscreensaver.conf```
 - To have nice menu settings also copy the .xml file ```cp hmx12multi.xml /usr/share/xscreensaver/config/```
 - Run the xscreensaver settings aka ```xscreensaver-demo```
 - The screensaver should be in the list

## Installation on a system with mate-screensaver (MATE Desktop)
 - Copy the executable "multi" to the screesaver dir ```sudo cp multi /usr/libexec/mate-screensaver/hmx12multi```
 - Edit the command line args at the Exec= line in hmx12multi.desktop to change the number of multis
 - Copy the .desktop file ```sudo cp hmx12multi.desktop /usr/share/applications/screensavers/```
 - Run the screensaver settings
 - The screensaver should be in the list (background will be black)

## Installation on a system with Other (Gnome/KDE)
 - Probably similar to mate, but I don't really know

## Standalone usage (like the gif above)
 - Run ```./xassign "./multi -d -n 10"``` where 10 is the number of multis you want
 - The cursor becomes a cross and allows to pick a window which will be overwritten (doesn't really work with all programs)
