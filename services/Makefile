all:
	$(MAKE) -C connectserver
	$(MAKE) -C mainserver
	$(MAKE) -C worldserver
	$(MAKE) -C joinserver
	$(MAKE) -C dataserver

dist:
	$(MAKE) -C connectserver install PREFIX=$(PREFIX)/bin
	$(MAKE) -C mainserver install PREFIX=$(PREFIX)/bin
	$(MAKE) -C worldserver install PREFIX=$(PREFIX)/bin
	$(MAKE) -C joinserver install PREFIX=$(PREFIX)/bin
	$(MAKE) -C dataserver install PREFIX=$(PREFIX)/bin

clean:
	$(MAKE) -C connectserver clean
	$(MAKE) -C mainserver clean
	$(MAKE) -C worldserver clean
	$(MAKE) -C joinserver clean
	$(MAKE) -C dataserver clean