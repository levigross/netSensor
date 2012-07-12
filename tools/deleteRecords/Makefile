include ../Makefile.inc

dumpPJL: ${DEPENDENCIES} deleteRecords.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra ${INCLUDES} \
		-I/usr/local/include/db5 \
		-L/usr/local/lib/db5 -ldb -o deleteRecords \
		deleteRecords.cpp ${LIBS}

clean:
	rm -f deleteRecords
