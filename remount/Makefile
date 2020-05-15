CC = cc
CFLAGS = -Wall
LDFLAGS =

EXEC = remount
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

CONFIGS = remount.conf.default
DESTDIR =
PREFIX = /usr/local

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

.PHONY: clean install_exec install_configs install uninstall

clean:
	rm -rf $(EXEC) $(OBJ)

install_exec: $(EXEC)
	install -d $(DESTDIR)$(PREFIX)/sbin
	install -m 4755 -o root -g root $^ $(DESTDIR)$(PREFIX)/sbin/

install_configs: $(CONFIGS)
	install -d $(DESTDIR)$(PREFIX)/share/remount
	install -m 0644 -o root -g root $^ $(DESTDIR)$(PREFIX)/share/remount/

install: install_exec install_configs

uninstall: clean
	install -d deleted-by-uninstall$(PREFIX)/sbin
	mv -f $(addprefix $(DESTDIR)$(PREFIX)/sbin/, $(EXEC)) deleted-by-uninstall$(PREFIX)/sbin/
	install -d deleted-by-uninstall$(PREFIX)/share/remount
	mv -f $(addprefix $(DESTDIR)$(PREFIX)/share/remount/, $(CONFIGS)) deleted-by-uninstall$(PREFIX)/share/remount/

