#./demo/Makefile

INCLUDE = -I $(PWD) \
		  -I $(PWD)/thirdparty/libuv/include \
		  -I $(PWD)/common/utilc \
		  -I $(PWD)/common/util \
		  -I $(PWD)/uvmodules/uvipc \
		  -I $(PWD)/uvmodules/uvlogplus \
		  -I $(PWD)/uvmodules/uvnetplus 

STATIC_LIB = -L $(OUT_DIR) \
             -l:uvnetplus.a \
			 -l:uvlogplus.a \
			 -l:util.a \
			 -l:utilc.a \
			 -l:libuv.a \
			 -l:cjson.a \
			 -l:pugixml.a \
			 -l:mbedtls.a \
			 -lrt -lresolv -lpthread -ldl

SHARED_LIB = -L $(OUT_DIR) \
             -l:libuv.so \
			 -l:cjson.so \
			 -l:pugixml.so \
			 -l:mbedtls.so \
			 -l:util.so \
			 -l:utilc.so \
			 -l:uvlogplus.so \
			 -l:uvnetplus.so \
			 -lrt -lresolv -lssl -lcrypto -lpthread -ldl

all:chkdir uvipctest$(TAGTYPE) \
           uvlogplustest$(TAGTYPE) \
		   uvnetplusftp$(TAGTYPE) \
		   uvnetplushttps$(TAGTYPE) \
		   uvnetplushttpsclient$(TAGTYPE) \
		   uvnetplustcptls$(TAGTYPE) \
		   uvnetplustest$(TAGTYPE)

chkdir:
	$(shell mkdir -p $(TMP_DIR)demo/)

uvipctest_static:uvIpcTest.c
	#$(CC) $(INCLUDE) $(CFLAGS) uvIpcTest.c -o $(OUT_DIR)uvipctest $(STATIC_LIB)

uvipctest_shared:uvIpcTest.c
	$(CC) $(INCLUDE) $(CFLAGS) uvIpcTest.c -o $(OUT_DIR)uvipctest $(SHARED_LIB)

###########################################################################

uvlogplustest_static:uvLogPlusTest.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvLogPlusTest.cpp -o $(OUT_DIR)uvlogplustest $(STATIC_LIB)

uvlogplustest_shared:uvLogPlusTest.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvLogPlusTest.cpp -o $(OUT_DIR)uvlogplustest $(SHARED_LIB)

#############################################################################

uvnetplusftp_static:uvNetPlusFtp.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusFtp.cpp -o $(OUT_DIR)uvnetplusftp $(STATIC_LIB)

uvnetplusftp_shared:uvNetPlusFtp.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusFtp.cpp -o $(OUT_DIR)uvnetplusftp $(SHARED_LIB)

#############################################################################

uvnetplushttps_static:uvNetPlusHttps.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusHttps.cpp -o $(OUT_DIR)uvnetplushttps $(STATIC_LIB)

uvnetplushttps_shared:uvNetPlusHttps.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusHttps.cpp -o $(OUT_DIR)uvnetplushttps $(SHARED_LIB)

#############################################################################

uvnetplushttpsclient_static:uvNetPlusHttpsClient.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusHttpsClient.cpp -o $(OUT_DIR)uvnetplushttpsclient $(STATIC_LIB)

uvnetplushttpsclient_shared:uvipctest.o
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusHttpsClient.cpp -o $(OUT_DIR)uvnetplushttpsclient $(SHARED_LIB)
	
#############################################################################

uvnetplustcptls_static:uvNetPlusTcpTls.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusTcpTls.cpp -o $(OUT_DIR)uvnetplustcptls $(STATIC_LIB)

uvnetplustcptls_shared:uvNetPlusTcpTls.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusTcpTls.cpp -o $(OUT_DIR)uvnetplustcptls $(SHARED_LIB)
	
#############################################################################

uvnetplustest_static:uvNetPlusTest.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusTcpTls.cpp -o $(OUT_DIR)uvnetplustest $(STATIC_LIB)

uvnetplustest_shared:uvNetPlusTest.cpp
	$(GG) $(INCLUDE) $(GFLAGS) uvNetPlusTcpTls.cpp -o $(OUT_DIR)uvnetplustest $(SHARED_LIB)
	
#############################################################################
	
clean:
	rm -rf $(TMP_DIR)demo/*.o