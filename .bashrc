# /etc/skel/.bashrc
#
# This file is sourced by all *interactive* bash shells on startup,
# including some apparently interactive shells such as scp and rcp
# that can't tolerate any output.  So make sure this doesn't display
# anything or bad things will happen !

# set more safe/conservative umask
umask 0027

# Test for an interactive shell.  There is no need to set anything
# past this point for scp and rcp, and it's important to refrain from
# outputting anything in those cases.
if [[ $- != *i* ]] ; then
  # Shell is non-interactive.  Be done now!
  return
fi

source $HOME/.bashrc.extern

echo -n 1 > /var/state/login

function uploadable {
  while read line; do
    sleep $((30 + $RANDOM % 15)) && plowdown --run-before=$HOME/local/plowhelp.sh "$line"
    if [ $? -ne 0 ]; then
      echo "$line" >> /tmp/plowdown-uploadable.list.failed
    else
      echo "$line" >> /tmp/plowdown-uploadable.list.done
    fi
  done
}

function pwk {
  curl -s "${1}" | git am
}

function weston-kill {
  pkill -x weston
}

function sensors {
  local node="/sys/class/thermal/thermal_zone0"

  if [ -e $node ]; then
    local type=$(cat $node/type)
    local temp=$(cat $node/temp)
    local degree=$(echo "scale=2; ${temp} / 1000" | bc)

    echo "${type}: ${degree} degree (celsius)"
  fi
}

function kmake {
  local numthreads=5

  if [ -n "${1}" ]; then
    make "${1}"
  fi

  make -j $numthreads
}

function exynos_vptest {
  local binpath="${HOME}/sourcecode/video/drm/tests/modetest/modetest"

  $binpath -M exynos -s 23@21:1920x1080@AR24 -P 21:640x480+128+128@NV12 -P 21:640x480+1024+128@RG16
}

function exynos_drmtest {
  local binpath="${HOME}/sourcecode/video/drm/tests/modetest/modetest"

  $binpath -M exynos -s 23@21:1920x1080@XR24 -P 21:640x480+1024+128@RG16
}

function exynos_rtest {
  #local execf="${HOME}/sourcecode/emu/rarch/RetroArch/retroarch"
  local execf="${HOME}/local/bin/retroarch"
  #local libpath="${HOME}/local/lib"
  local libpath=$HOME/sourcecode/video/drm/exynos/.libs:$HOME/local/lib
  local raconf
  local racontent

  if [ "${psx:-0}" -eq 1 ]; then
    raconf="${HOME}/local/pcsx.libretro"
    racontent="${HOME}/emulation/psx/cdimages/Silent.Hill.US.NTSC.iso"
  else
    raconf="${HOME}/local/snes9x.libretro"
    racontent="${HOME}/emulation/snes/roms/Seiken Densetsu 3 (English) (Translated).sfc"
  fi

  if [ "${gdb:-0}" -eq 1 ]; then
    LD_LIBRARY_PATH=$libpath gdb --args "${execf}" -v -c "${raconf}" "${racontent}"
  else
    LD_LIBRARY_PATH=$libpath "${execf}" -v -c "${raconf}" "${racontent}"
  fi
}
