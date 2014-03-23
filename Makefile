
bld/xbee:	xbee.cpp
	mkdir -p bld
	g++ -MMD -Wall -Werror -pipe -g -c -o bld/xbee.o xbee.cpp
	g++ -MMD -Wall -Werror -pipe -g -o bld/xbee bld/xbee.o -lstdc++

clean:
	rm -fr bld
