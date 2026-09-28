#!/bin/bash
# Build wrapper for our custom kitty fork
export PKG_CONFIG_PATH="$HOME/quake-terminal/pkgconfig-shims${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export PATH="$HOME/quake-terminal/tools-bin:$HOME/quake-terminal/docs-venv/bin:$PATH"
cd "$HOME/quake-terminal/kitty"
exec make -j8 "$@"
