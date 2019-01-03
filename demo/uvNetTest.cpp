// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "uvHttp.h"
#include "dns.h"
#include <windows.h>

void on_dns_lookup(dns_resolver_t* res, int err, char *address, int family) {
    printf("err:%d, addr:%s, family:%d \n",err, address, family); 
}

int _tmain(int argc, _TCHAR* argv[])
{
	config_t cof;
	cof.keep_alive_secs = 0;
	cof.max_sockets = 10;
	cof.max_free_sockets = 10;
    http_t* h = uvHttp(cof, NULL);
    dns_resolver_t* dns = dns_create_resolver(h);
    dns_lookup(dns, "www.baidu.com", on_dns_lookup);
    dns_lookup(dns, "www.baidu.com", on_dns_lookup);
    dns_lookup(dns, "www.hahah.222", on_dns_lookup);
    dns_lookup(dns, "localhost", on_dns_lookup);
	Sleep(INFINITE);
	return 0;
}

