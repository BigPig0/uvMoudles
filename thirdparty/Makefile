#./ThirdParty/Makefile
SUBDIR = mbedtls #cjson libcstl libuv pugixml

all:$(SUBDIR)
	
$(SUBDIR):ECHO
	make -C $@

ECHO:  
	@echo $@

.PHONY : clean

clean :CLEANDIR

CLEANDIR:ECHO
	make -C $(SUBDIR) clean