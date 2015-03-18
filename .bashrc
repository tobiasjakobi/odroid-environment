# /etc/skel/.bashrc
#
# This file is sourced by all *interactive* bash shells on startup,
# including some apparently interactive shells such as scp and rcp
# that can't tolerate any output.  So make sure this doesn't display
# anything or bad things will happen !


# Test for an interactive shell.  There is no need to set anything
# past this point for scp and rcp, and it's important to refrain from
# outputting anything in those cases.
if [[ $- != *i* ]] ; then
  # Shell is non-interactive.  Be done now!
  return
fi

source $HOME/.bashrc.extern

echo -n 1 > /var/state/login

# XDG setup for wayland/weston
if [ -z "${XDG_RUNTIME_DIR}" ]; then
  export XDG_RUNTIME_DIR=/tmp/${UID}-runtime-dir
  if [ ! -d "${XDG_RUNTIME_DIR}" ]; then
    mkdir "${XDG_RUNTIME_DIR}"
    chmod 0700 "${XDG_RUNTIME_DIR}"
  fi
fi

function pwk {
  curl -s "${1}" | git am
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
  local loadaddr=0x40008000

  if [ -n "${1}" ]; then
    make "${1}"
  fi

  make -j $numthreads && \
    make LOADADDR=$loadaddr uImage
}

function exynos_rtest {
  local execf="${HOME}/sourcecode/emu/rarch/RetroArch/retroarch"
  #local libpath="${HOME}/local/lib"
  local libpath=$HOME/sourcecode/video/drm/exynos/.libs:$HOME/local/lib
  local raconf
  local racontent

  if [ "${psx}" -eq 1 ]; then
    raconf="${HOME}/local/pcsx.libretro"
    racontent="${HOME}/emulation/psx/cdimages/Silent.Hill.US.NTSC.iso"
  else
    raconf="${HOME}/local/snes9x.libretro"
    racontent="${HOME}/emulation/snes/roms/Seiken Densetsu 3 (English) (Translated).sfc"
  fi

  if [ "${gdb}" -eq 1 ]; then
    LD_LIBRARY_PATH=$libpath gdb --args "${execf}" -v -c "${raconf}" "${racontent}"
  else
    LD_LIBRARY_PATH=$libpath "${execf}" -v -c "${raconf}" "${racontent}"
  fi
}
