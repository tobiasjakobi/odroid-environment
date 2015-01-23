#!/bin/sh
#
# Executed by startx

if [ -d /etc/X11/xinit/xinitrc.d ]; then
  for f in /etc/X11/xinit/xinitrc.d/*; do
    [ -x "$f" ] && . "$f"
  done
  unset f
fi

echo -n 1 > /var/state/login

case $1 in
  xbmc ) exec $HOME/local/bin/xbmc ;;
  * )    exec /usr/bin/xterm ;;
esac
