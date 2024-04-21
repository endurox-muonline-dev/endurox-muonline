SOURCES=Exfields MuOnline.fd

all: 
	@echo "========================"	
	@echo "Generating fielded buffer headers"
	@mkdir -p  $(OUTPUTDIR)
	@mkfldhdr -m0 -pubftab $(SOURCES) -d $(OUTPUTDIR)
	@echo "========================"

clean:
	@-rm -f $(OUTPUTDIR)/*.h

dist:
	@mkdir -p $(PREFIX)
	install $(SOURCES) $(PREFIX)