include ../Makefile.inc

countPJL: ${DEPENDENCIES} countPJL.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra ${INCLUDES} \
		-I/usr/local/include/db5 \
		-L/usr/local/lib/db5 -ldb -o countPJL \
		countPJL.cpp ${LIBS}

clean:
	rm -f countPJL
