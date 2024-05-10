CFLAGS := -W -Wall

all::
	$(CC) $(CFLAGS) wldd.c -o wldd -lelf
	cp wldd ../../bin/

clean:
	rm -f wldd

install:
	cp wldd $(DESTDIR)/usr/bin/wldd

uninstall:
	rm $(DESTDIR)/usr/bin/wldd -f

