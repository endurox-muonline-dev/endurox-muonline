all:
	$(MAKE) -C connectserver
#	$(MAKE) -C gameserver

dist:
	$(MAKE) -C connectserver install PREFIX=$(PREFIX)/bin
#	$(MAKE) -C gameserver install PREFIX=$(PREFIX)/bin

clean:
	$(MAKE) -C connectserver clean
#	$(MAKE) -C gameserver clean