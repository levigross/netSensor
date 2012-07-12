SUBDIRS=shared sensor tools

all: ${SUBDIRS} Makefile
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" $$subdir "($@)"; export dir=$$subdir/; make); done

clean:
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" $$subdir "($@)"; export dir=$$subdir/; make clean); done
