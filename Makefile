all:
	cd src && make

clean:
	cd src && make clean
	rm -f ./bin/*

install:
	cd src && make install

uninstall:
	cd src && make uninstall

