#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2022 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

all::
	cp wldd ../../bin/
	cp wcch ../../bin/
test:

clean:
	rm ../../bin/wldd -f
	rm ../../bin/wcch -f

install:
	cp wldd $(DESTDIR)/usr/bin/wldd
	cp wcch $(DESTDIR)/usr/bin/wcch

uninstall:
	rm $(DESTDIR)/usr/bin/wldd -f
	rm $(DESTDIR)/usr/bin/wcch -f



