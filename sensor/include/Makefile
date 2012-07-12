DEPENDENCIES=../../shared/include/*
INCLUDES=-I../../shared -I..

all: berkeleyDB.o configuration.o endian.o ethernetInfo.o flowID.o \
		httpParser.o httpSession.o logger.o module.o packet.o smtp.o \
		Makefile
	ar rcs ../lib/sensor.a *.o

berkeleyDB.o: berkeleyDB.h berkeleyDB.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -I/usr/local/include/db5 \
		-I/opt/local/include/db44 -c -o berkeleyDB.o berkeleyDB.cpp

configuration.o: configuration.h configuration.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c -o configuration.o \
		configuration.cpp

endian.o: endian.h endian.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c -o endian.o endian.cpp

ethernetInfo.o: ethernetInfo.h ethernetInfo.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c -o ethernetInfo.o \
		ethernetInfo.cpp

flowID.o: ${DEPENDENCIES} flowID.h flowID.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} -o flowID.o \
		flowID.cpp

httpParser.o: httpParser.h httpParser.c Makefile
	${CC} ${CFLAGS} -Wall -Wextra -fPIC -c -o httpParser.o httpParser.c

httpSession.o: ${DEPENDENCIES} httpSession.h httpSession.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} -o \
		httpSession.o httpSession.cpp

logger.o: logger.h logger.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c -o logger.o logger.cpp

module.o: ${DEPENDENCIES} module.h module.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} -o module.o \
		module.cpp

packet.o: ${DEPENDENCIES} packet.h packet.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} -o packet.o \
		packet.cpp

smtp.o: ${DEPENDENCIES} smtp.h smtp.cpp Makefile
	${CXX} ${CXXFLAGS} -Wall -Wextra -fPIC -c ${INCLUDES} \
		-I/usr/local/include -I/opt/local/include -o smtp.o smtp.cpp

clean:
	rm -f *.o ../lib/sensor.a
