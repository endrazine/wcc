#
# This file is part of the Witchcraft Compiler Collection
# Copyright 2016-2022 Jonathan Brossard
#
# Homepage: https://github.com/endrazine/wcc/
#
# This file is licensed under MIT License.
#

all::
	cd wcc && make CFLAGS=" $(CFLAGS)"
	cd wld && make CFLAGS=" $(CFLAGS)"
	cd wsh && make CFLAGS=" $(CFLAGS)"
	cd tools && make CFLAGS=" $(CFLAGS)"
clean:
	cd wcc && make clean
	cd wld && make clean
	cd wsh && make clean
	cd tools && make clean

install:
	cd wcc && make install
	cd wld && make install
	cd wsh && make install
	cd tools && make install

uninstall:
	cd wcc && make uninstall
	cd wld && make uninstall
	cd wsh && make uninstall
	cd tools && make uninstall

