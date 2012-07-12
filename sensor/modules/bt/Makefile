include ../Makefile.inc

bt.so: udpTrackerSession.h udpTrackerSession.cpp bt.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC ${INCLUDES} \
		-c udpTrackerSession.cpp
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC ${INCLUDES} \
		-I/usr/local/include -c bt.cpp
	${CXX} ${CXXFLAGS} -Wall -Wextra -shared -lesmtp -L/usr/local/lib \
		-o bt.so udpTrackerSession.o bt.o ${LIBS}

clean:
	rm -f udpTrackerSession.o bt.o bt.so
