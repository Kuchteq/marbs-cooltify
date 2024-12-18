# Optimize, turn on additional warnings
CFLAGS ?= -std=c17 
PKG_CONFIG ?= pkg-config

WAYLAND_FLAGS = $(shell $(PKG_CONFIG) wayland-client --cflags --libs)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) wayland-protocols --variable=pkgdatadir)

# Build deps
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

HEADERS=xdg-shell-client-protocol.h wlr-layer-shell-unstable-v1-protocol.h shm.h draw_help.h
SOURCES=main.c xdg-shell-protocol.c wlr-layer-shell-unstable-v1-protocol.c shm.c draw_help.c
.PHONY: all
all: cooltify
CFLAGS+=$(shell pkg-config --cflags wayland-client wayland-cursor fcft pixman-1 xkbcommon)
LDLIBS+=$(shell pkg-config --libs wayland-client wayland-cursor fcft pixman-1 xkbcommon) -lrt -lm

cooltify: $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(WAYLAND_FLAGS) -lxkbcommon -lfcft -lpixman-1

xdg-shell-client-protocol.h:
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) xdg-shell-client-protocol.h

xdg-shell-protocol.c:
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) xdg-shell-protocol.c


wlr-layer-shell-unstable-v1-protocol.h:
	$(WAYLAND_SCANNER) client-header protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.c:
	$(WAYLAND_SCANNER) private-code protocols/wlr-layer-shell-unstable-v1.xml $@
wlr-layer-shell-unstable-v1-protocol.o: wlr-layer-shell-unstable-v1-protocol.h

install: cooltify
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 0755 cooltify $(DESTDIR)$(PREFIX)/bin/cooltify

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/cooltify

.PHONY: clean
clean:
	rm -f main
