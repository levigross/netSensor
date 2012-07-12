include ../Makefile.inc

dumpPJL: ${DEPENDENCIES} dumpPJL.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra ${INCLUDES} \
		-I/usr/local/include/db5 \
		-L/usr/local/lib/db5 -ldb -o dumpPJL \
		dumpPJL.cpp ${LIBS}

clean:
	rm -f dumpPJL
