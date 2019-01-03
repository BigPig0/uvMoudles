/*!
 * \file dns.h
 * \date 2018/12/19 18:07
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * Node.js v11.5.0 Documentation
 *
 * Table of Contents
 DNS

 Class: dns.Resolver

 resolver.cancel()
 dns.getServers()
 dns.lookup(hostname[, options], callback)

 Supported getaddrinfo flags
 dns.lookupService(address, port, callback)
 dns.resolve(hostname[, rrtype], callback)
 dns.resolve4(hostname[, options], callback)
 dns.resolve6(hostname[, options], callback)
 dns.resolveAny(hostname, callback)
 dns.resolveCname(hostname, callback)
 dns.resolveMx(hostname, callback)
 dns.resolveNaptr(hostname, callback)
 dns.resolveNs(hostname, callback)
 dns.resolvePtr(hostname, callback)
 dns.resolveSoa(hostname, callback)
 dns.resolveSrv(hostname, callback)
 dns.resolveTxt(hostname, callback)
 dns.reverse(ip, callback)
 dns.setServers(servers)


 dns.lookup()
 dns.resolve(), dns.resolve*() and dns.reverse()
*/


#ifndef _UV_DNS_
#define _UV_DNS_ 

#ifdef __cplusplus
extern "C" {
#endif

/**
* Supported getaddrinfo flags
The following flags can be passed as hints to dns.lookup().

dns.ADDRCONFIG: Returned address types are determined by the types of addresses supported by the current system. For example, IPv4 addresses are only returned if the current system has at least one IPv4 address configured. Loopback addresses are not considered.
dns.V4MAPPED: If the IPv6 family was specified, but no IPv6 addresses were found, then return IPv4 mapped IPv6 addresses. Note that it is not supported on some operating systems (e.g FreeBSD 10.1).
 */
#define DNS_GETADDRINFO_TAG_ADDRCONFIG 1
#define DNS_GETADDRINFO_TAG_V4MAPPED   2

enum _dns_error_code_ {
    DNS_ERROR_SUCCESS = 0,
    DNS_ERROR_NODATA,
    DNS_ERROR_FORMERR,
    DNS_ERROR_SERVFAIL,
    DNS_ERROR_NOTFOUND,
    DNS_ERROR_NOTIMP,
    DNS_ERROR_REFUSED,
    DNS_ERROR_BADQUERY,
    DNS_ERROR_BADNAME,
    DNS_ERROR_BADFAMILY,
    DNS_ERROR_BADRESP,
    DNS_ERROR_CONNREFUSED,
    DNS_ERROR_TIMEOUT,
    DNS_ERROR_EOF,
    DNS_ERROR_FILE,
    DNS_ERROR_NOMEM,
    DNS_ERROR_DESTRUCTION,
    DNS_ERROR_BADSTR,
    DNS_ERROR_BADFLAGS,
    DNS_ERROR_NONAME,
    DNS_ERROR_BADHINTS,
    DNS_ERROR_NOTINITIALIZED,
    DNS_ERROR_LOADIPHLPAPI,
    DNS_ERROR_ADDRGETNETWORKPARAMS,
    DNS_ERROR_CANCELLED,
    DNS_ERROR_MAX
}dns_error_code_t;

typedef struct _dns_lookup_options_ {
    int    family;   //The record family. Must be 4 or 6. IPv4 and IPv6 addresses are both returned by default.
    int    hints;    //One or more supported getaddrinfo flags. Multiple flags may be passed by bitwise ORing their values.
    bool   all;      //When true, the callback returns all resolved addresses in an array. Otherwise, returns a single address. Default: false.
    bool   verbatim; //When true, the callback receives IPv4 and IPv6 addresses in the order the DNS resolver returned them. When false, IPv4 addresses are placed before IPv6 addresses. Default: currently false (addresses are reordered) but this is expected to change in the not too distant future. New code should use { verbatim: true }.
}dns_lookup_options_t;
extern dns_lookup_options_t* dns_create_lookup_options();

typedef struct _dns_resolver_ dns_resolver_t;

/**
 * Class: dns.Resolver
 * An independent resolver for DNS requests.
 * Note that creating a new resolver uses the default server settings. Setting the servers used for a resolver using resolver.setServers() does not affect other resolvers:
 */
extern dns_resolver_t* dns_create_resolver(http_t *handle);
extern void dns_destory_resolver(dns_resolver_t* res);

/**
 * 绑定用户数据
 */
extern void dns_set_data(dns_resolver_t* res, void* user_data);
extern void* dns_get_data(dns_resolver_t* res);

/**
 * resolver.cancel()
 * Cancel all outstanding DNS queries made by this resolver. The corresponding callbacks will be called with an error with code ECANCELLED.
 */
extern void dns_resolver_cancel(dns_resolver_t* res);

/**
 * dns.getServers()
 * Returns: <string[]>
 * Returns an array of IP address strings, formatted according to rfc5952, that are currently configured for DNS resolution. A string will include a port section if a custom port is used.
 */
extern int dns_resolver_get_servers(dns_resolver_t* res, char** servers);

/**
 * dns.lookup(hostname[, options], callback)
 * hostname <string>
 * options <integer> | <Object>
 *    family <integer> The record family. Must be 4 or 6. IPv4 and IPv6 addresses are both returned by default.
 *    hints <number> One or more supported getaddrinfo flags. Multiple flags may be passed by bitwise ORing their values.
 *    all <boolean> When true, the callback returns all resolved addresses in an array. Otherwise, returns a single address. Default: false.
 *    verbatim <boolean> When true, the callback receives IPv4 and IPv6 addresses in the order the DNS resolver returned them. When false, IPv4 addresses are placed before IPv6 addresses. Default: currently false (addresses are reordered) but this is expected to change in the not too distant future. New code should use { verbatim: true }.
 * callback <Function>
 *    err <Error>
 *    address <string> A string representation of an IPv4 or IPv6 address.
 *    family <integer> 4 or 6, denoting the family of address.
 * Resolves a hostname (e.g. 'nodejs.org') into the first found A (IPv4) or AAAA (IPv6) record. All option properties are optional. If options is an integer, then it must be 4 or 6 – if options is not provided, then IPv4 and IPv6 addresses are both returned if found.

 * With the all option set to true, the arguments for callback change to (err, addresses), with addresses being an array of objects with the properties address and family.
 * On error, err is an Error object, where err.code is the error code. Keep in mind that err.code will be set to 'ENOENT' not only when the hostname does not exist but also when the lookup fails in other ways such as no available file descriptors.
 * dns.lookup() does not necessarily have anything to do with the DNS protocol. The implementation uses an operating system facility that can associate names with addresses, and vice versa. This implementation can have subtle but important consequences on the behavior of any Node.js program. Please take some time to consult the Implementation considerations section before using dns.lookup().
 */
typedef void (*dns_lookup_cb)(dns_resolver_t* res, int err, char *address, int family);
extern void dns_lookup(dns_resolver_t* res, char* hostname, dns_lookup_cb cb);
extern void dns_lookup_options(dns_resolver_t* res, char* hostname, dns_lookup_options_t *options, dns_lookup_cb cb);
extern void dns_lookup_family(dns_resolver_t* res, char* hostname, int family, dns_lookup_cb cb);

/**
 * dns.lookupService(address, port, callback)
 * address <string>
 * port <number>
 * callback <Function>
 *     err <Error>
 *     hostname <string> e.g. example.com
 *     service <string> e.g. http
 * Resolves the given address and port into a hostname and service using the operating system's underlying getnameinfo implementation.
 *
 * If address is not a valid IP address, a TypeError will be thrown. The port will be coerced to a number. If it is not a legal port, a TypeError will be thrown.
 * On an error, err is an Error object, where err.code is the error code.
 */
typedef void (*dns_lookup_service_cb)(dns_resolver_t* res, int err, char *hostname, char *service);
extern void dns_lookup_service(dns_resolver_t* res, char *address, int port, dns_lookup_service_cb cb);

/**
 * dns.setServers(servers)
 * servers <string[]> array of rfc5952 formatted addresses
 * Sets the IP address and port of servers to be used when performing DNS resolution. The servers argument is an array of rfc5952 formatted addresses. If the port is the IANA default DNS port (53) it can be omitted.
 *
 * dns.setServers([
 *     '4.4.4.4',
 *     '[2001:4860:4860::8888]',
 *     '4.4.4.4:1053',
 *     '[2001:4860:4860::8888]:1053']);
 *
 * An error will be thrown if an invalid address is provided.
 * The dns.setServers() method must not be called while a DNS query is in progress.
 * Note that this method works much like resolve.conf. That is, if attempting to resolve with the first server provided results in a NOTFOUND error, the resolve() method will not attempt to resolve with subsequent servers provided. Fallback DNS servers will only be used if the earlier ones time out or result in some other error.
 */
extern void dns_set_servers(dns_resolver_t* res, char **servers, int num);

#ifdef __cplusplus
}
#endif

#endif