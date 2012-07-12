SUBDIRS=include

all: ${SUBDIRS} Makefile
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" ${dir}$$subdir "($@)"; make); done

clean:
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" ${dir}$$subdir "($@)"; make clean); done
