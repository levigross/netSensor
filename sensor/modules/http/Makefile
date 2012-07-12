include ../Makefile.inc

http.so: http.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC ${INCLUDES} \
		-I/usr/local/include/db5 -c http.cpp
	${CXX} ${CXXFLAGS} -Wall -Wextra -shared -o http.so http.o ${LIBS}

clean:
	rm -f http.o http.so
