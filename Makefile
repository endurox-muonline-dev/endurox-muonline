MU_MODULE_ROOT=$(shell pwd)
export MU_MODULE_ROOT

# Prefix where to copy files, default it's `pwd`\dist
ifndef PREFIX
	PREFIX=`pwd`/dist
endif

all:
	$(MAKE) -C ubftab OUTPUTDIR=$(MU_MODULE_ROOT)/ubftab/include
#	$(MAKE) -C libmuonline BASE_DIR=`pwd`
#	$(MAKE) -C libpgdrv BASE_DIR=`pwd`
	$(MAKE) -C services BASE_DIR=`pwd`

dist:
	@rm -rf ./dist
	$(MAKE) -C ubftab dist PREFIX=$(PREFIX)/ubftab
	$(MAKE) -C services dist PREFIX=$(PREFIX)

clean:
	$(MAKE) -C ubftab clean OUTPUTDIR=$(MU_MODULE_ROOT)/ubftab/include
#	$(MAKE) -C libmuonline clean
#	$(MAKE) -C libpgdrv clean
	$(MAKE) -C services clean
	@rm -fr ./dist

rpm:
	./build_rpms.sh

.PHONY: dist