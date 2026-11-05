#pragma once
#include <cstdint>

namespace megusta::engine_socket {

// The recovered poll record is 8 bytes in the 32-bit executable.
struct PollEntry {
    std::int32_t socket;
    std::uint16_t events;
    std::uint16_t returnedEvents;
};
constexpr std::uint16_t pollRead=1;
constexpr std::uint16_t pollWrite=2;

int engineSocket_accept_adapter(int socket,void* address,int* addressLength);
void engineSocket_translateLastError_impl();
int engineSocket_bind_adapter(int socket,const void* address,int addressLength);
int engineSocket_close_adapter(int socket);
int engineSocket_connect_adapter(int socket,const void* address,int addressLength);
int engineSocket_listen_adapter(int socket,int backlog);
int engineSocket_recv_adapter(int socket,char* buffer,int length,int flags);
int engineSocket_recvfrom_adapter(int socket,char* buffer,int length,int flags,void* address,int* addressLength);
int engineSocket_send_adapter(int socket,const char* buffer,int length,int flags);
int engineSocket_sendto_adapter(int socket,const char* buffer,int length,int flags,const void* address);
void engineSocket_shutdown_adapter(int socket,int how);
int engineSocket_create_adapter(int domain,int type,int protocol);
std::uint32_t engineSocket_inetAddr_adapter(const char* address);
char* engineSocket_inetNtoa_adapter(std::uint32_t address);
int engineSocket_getsockoptUnsupported_impl();
int engineSocket_setOptionOrNonblocking_impl(int socket,int unused,int option,const int* value,int length);
std::uint16_t engineSocket_ntohs_adapter(std::uint16_t value);
std::uint32_t engineNet_hostToNetwork32_adapter(std::uint32_t value);
std::uint16_t engineNet_hostToNetwork16_adapter(std::uint16_t value);
std::uint32_t engineNet_networkToHost32_adapter(std::uint32_t value);
int engineNet_initializeWinsock_impl();
void engineNet_shutdownWinsock_impl();
int engineSocket_pollWithSelect_impl(PollEntry* entries,int count,int timeoutMilliseconds);
int engineSocket_gethostname_adapter(char* name,int length);
int engineSocket_getsockname_adapter(int socket,void* address,int* addressLength);
void* engineSocket_gethostbyaddr_adapter(const char* address,int length,int family);
void* engineSocket_gethostbyname_adapter(const char* name);
void* engineSocket_getprotobyname_adapter(const char* name);
void* engineSocket_getservbyname_adapter(const char* name,const char* protocol);

} // namespace megusta::engine_socket
