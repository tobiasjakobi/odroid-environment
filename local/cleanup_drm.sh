#!/bin/bash

function cleanup_libraries {
  local base="${HOME}/local/lib"

  rm "${base}"/libkms.la "${base}"/libkms.so*
  rm "${base}"/pkgconfig/libkms.pc
}

function cleanup_headers {
  local base="${HOME}/local/include"

  rm "${base}"/libkms/*.h && rmdir "${base}"/libkms

  for arg in amdgpu i915 mach64 mga nouveau qxl r128 \
             radeon savage sis tegra vc4 via; do
    rm "${base}"/libdrm/"${arg}_drm.h"
  done
}

cleanup_libraries
cleanup_headers
