include ../Makefile.inc

all: berkeleyDB.o options.o
	ar rcs ../lib/tools.a *.o

berkeleyDB.o: berkeleyDB.h berkeleyDB.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} \
		-I/usr/local/include/db5 berkeleyDB.cpp

options.o: ${DEPENDENCIES} options.h options.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} -c options.cpp

clean: 
	rm -f *.o ../lib/tools.a
