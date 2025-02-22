# ----------------------------
# Makefile Options
# ----------------------------

SHELL=/bin/zsh

NAME = OWLEYES
ICON = icon.png
DESCRIPTION = "Lorenz system plotter"
COMPRESSED = NO
CFLAGS = -Wall -Wextra -O3
CXXFLAGS = -Wall -Wextra -O3

# ----------------------------

include $(shell cedev-config --makefile)
