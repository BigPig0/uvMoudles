#./ThirdParty/Makefile
SUBDIR = cjson libuv pugixml lua exosip ocilib hiredis_linux

all:$(SUBDIR)
	
$(SUBDIR):ECHO
	make -C $@

ECHO:  
	@echo $@

.PHONY : clean

clean :CLEANDIR

CLEANDIR:ECHO
	make -C $(SUBDIR) clean