#!/bin/bash

# Prevent shutdown when RetroArch is running.
echo -n 1 > /var/state/login

source $HOME/local/bin/retroarch_scripts

# Configure audio (intern vs. USB) and input (single vs. multi).
rarch_config_audio
rarch_config_input

# Don't run the emulator if there is no gamepad attached.
rarch_check_gamepad || exit 1

rarch_content=$(rarch_select_content "${1}")

retroarch "${1}" "${rarch_content}"
