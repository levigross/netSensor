SUBDIRS=include modules

all: ${SUBDIRS} Makefile
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" \
		${dir}$$subdir "($@)"; export dir=${dir}$$subdir/; make); done
	@echo "===> sensor ($@)"
	@make sensor

clean:
	@for subdir in ${SUBDIRS}; do (cd $$subdir; echo "===>" \
		${dir}$$subdir "($@)"; export dir=${dir}$$subdir/; make clean); done
	@echo "===> sensor ($@)"
	@make clean-sensor

include Makefile.sensor
