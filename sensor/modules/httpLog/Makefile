include ../Makefile.inc

httpLog.so: httpLog.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC ${INCLUDES} \
		-I/usr/local/include/db5 -c httpLog.cpp
	${CXX} ${CXXFLAGS} -Wall -Wextra -shared -ldb -L/usr/local/lib/db5 \
		-o httpLog.so httpLog.o ${LIBS}

clean:
	rm -f httpLog.o httpLog.so
