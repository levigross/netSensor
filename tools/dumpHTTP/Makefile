include ../Makefile.inc

dumpHTTP: ${DEPENDENCIES} message.hpp dumpHTTP.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra ${INCLUDES} \
		-I/usr/local/include/db5 \
		-L/usr/local/lib/db5 -ldb -o dumpHTTP \
		dumpHTTP.cpp ${LIBS}

clean:
	rm -f dumpHTTP
