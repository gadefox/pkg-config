-include config.mk

CFLAGS += -Wall -std=c99 -pedantic -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809L

all: config.mk outdir pkg-config

config.mk:
	@if ! test -e config.mk; then printf "\033[31;1mERROR:\033[0m you have to run ./configure\n"; exit 1; fi

OBJ = out/cflags.o \
			out/flag.o \
			out/globals.o \
			out/libs.o \
			out/main.o \
			out/package.o \
			out/parse.o \
			out/reqver.o \
			out/strutil.o \
			out/taillist.o \
			out/utils.o

$(OBJ):
	$(QUIET_CC)$(CC) $(CFLAGS) -c src/$(@F:.o=.c) -o $@

pkg-config: $(OBJ)
	$(QUIET_LINK)$(CC) $^ $(LIBS) -o out/$@

outdir:
	@mkdir -p out

install:
	@echo installing pkg-config 
	@mkdir -p $(DESTDIR)$(BIN_DIR)
	@cp -f out/pkg-config $(DESTDIR)$(BIN_DIR)
	@strip -s $(DESTDIR)$(BIN_DIR)/pkg-config
	@chmod 755 $(DESTDIR)$(BIN_DIR)/pkg-config
	@echo installing pkg-config.pc
	@mkdir -p $(DESTDIR)$(SHARE_DIR)/pkg-config
	@cp -f data/pkg-config.pc $(DESTDIR)$(SHARE_DIR)/pkg-config
	@chmod 644 $(DESTDIR)$(SHARE_DIR)/pkg-config/pkg-config.pc
	@echo installing manual
	@mkdir -p $(DESTDIR)$(MAN_DIR)
	@cp -f data/pkg-config.1 $(DESTDIR)$(MAN_DIR)
	@gzip -f -9 $(DESTDIR)$(MAN_DIR)/pkg-config.1
	@chmod 644 $(DESTDIR)$(MAN_DIR)/pkg-config.1.gz

uninstall:
	@echo uninstalling pkg-config
	@rm -f $(DESTDIR)$(BIN_DIR)/pkg-config
	@echo uninstalling pkg-config.pc
	@rm -f $(DESTDIR)$(SHARE_DIR)/pkg-config/pkg-config.pc
	@echo uninstalling manual
	@rm -f $(DESTDIR)$(MAN_DIR)/pkg-config.1.gz

clean:
	@echo removing pkg-config output files..
	@rm -f out/*.o
	@rm -f

distclean: clean
	@echo removing config.mk include file
	@rm -f config.mk

check:
	@test/check-cflags
	@test/check-libs
	@test/check-mixed-flags
	@test/check-non-l-flags
	@test/check-define-variable
	@test/check-libs-private
	@test/check-requires-private
	@test/check-circular-requires
	@test/check-includedir
	@test/check-conflicts
	@test/check-missing
	@test/check-special-flags
	@test/check-sort-order
	@test/check-duplicate-flags
	@test/check-whitespace
	@test/check-cmd-options
	@test/check-version
	@test/check-requires-version
	@test/check-print-options
	@test/check-path
	@test/check-sysroot
	@test/check-uninstalled
	@test/check-debug
	@test/check-gtk
	@test/check-tilde
	@test/check-relocatable
	@test/check-variable-override
	@test/check-variables
	@test/check-dependencies
	@test/check-system-flags

.PHONY: all clean distclean install uninstall check
