CC	:=	gcc
CFLAGS	:=	-W -Wall
LDFLAGS	:=	-ldl -T script.lds

all::
	cp /usr/sbin/proftpd /tmp
	wld -libify /tmp/proftpd
	mv /tmp/proftpd /tmp/proftpd.so
	$(CC) $(CFLAGS) demo0.c -o demo0 $(LDFLAGS)
	$(CC) $(CFLAGS) demo1.c -o demo1 $(LDFLAGS)
	$(CC) $(CFLAGS) demo2.c -o demo2 $(LDFLAGS)
	$(CC) $(CFLAGS) demo3.c -o demo3 $(LDFLAGS)

clean::
	rm demo0 demo1 demo2 demo3

