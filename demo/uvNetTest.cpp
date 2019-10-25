// uvHttpClient.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <tchar.h>
#include "uvnode.h"
#include "dns.h"
#include "net.h"
#include <windows.h>

void on_dns_lookup(dns_resolver_t* res, int err, char *address, int family) {
    printf("err:%d, addr:%s, family:%d \n",err, address, family); 
}

void on_socket_write(net_socket_t* skt){
    net_address_t cli = net_socket_address(skt);
    printf("on write %s %d %d\n",cli.address, cli.port, cli.family);
}

void on_on_socket_event_data(net_socket_t* skt, char *data, int len) {
    net_address_t cli = net_socket_address(skt);
    printf("on data %s %d %d len:%d data:%s\n",cli.address, cli.port, cli.family, len, data);
    static char *buff = "Answer";
    net_socket_write(skt, buff, strlen(buff), on_socket_write);
}

void on_socket_event_end(net_socket_t* skt){
    net_address_t cli = net_socket_address(skt);
    printf("on end %s %d %d\n",cli.address, cli.port, cli.family);
}

void on_on_server_event_connection(net_server_t* svr, int err, net_socket_t* client) {
    net_address_t cli = net_socket_address(client);
    printf("on connection %s %d %d\n",cli.address, cli.port, cli.family);

    net_socket_on_data(client, on_on_socket_event_data);
    net_socket_on_end(client, on_socket_event_end);

    for(int i=0; i<10; i++){
    static char *buff = "server to client";
    net_socket_write(client, buff, strlen(buff), on_socket_write);
    Sleep(100);
    }
}

void on_on_server_listen(net_server_t* svr, int err) {
    net_address_t cli;
    //net_server_address(svr, &cli);
    //printf("on listen %s %d %d\n",cli.address, cli.port, cli.family);
}

void on_socket_connect(net_socket_t* skt) {
    net_address_t cli = net_socket_address(skt);
    printf("on connect %s %d %d\n",cli.address, cli.port, cli.family);
    net_socket_on_data(skt, on_on_socket_event_data);
    static char *buff = "Request";
    net_socket_end(skt, buff, strlen(buff), on_socket_write);
}

int _tmain(int argc, _TCHAR* argv[])
{
	config_t cof;
	cof.keep_alive_secs = 0;
	cof.max_sockets = 10;
	cof.max_free_sockets = 10;
    uv_node_t* h = uv_node_create(cof, NULL);
#if 0
    dns_resolver_t* dns = dns_create_resolver(h);
    dns_lookup(dns, "www.baidu.com", on_dns_lookup);
    dns_lookup(dns, "www.baidu.com", on_dns_lookup);
    dns_lookup(dns, "www.hahah.222", on_dns_lookup);
    dns_lookup(dns, "localhost", on_dns_lookup);
#endif

#if 1
    net_server_options_t option = {true, false};
    net_server_t* svr = net_create_server(h, &option, on_on_server_event_connection);
    net_server_listen_port(svr, 8080, NULL, 0, on_on_server_listen);

    net_socket_options_t options = {0, true, false, false};
    net_socket_t* clt = net_create_socket(h, &options);
    net_socket_connect_port(clt, 8080, NULL, on_socket_connect);

#endif
	Sleep(INFINITE);
	return 0;
}

