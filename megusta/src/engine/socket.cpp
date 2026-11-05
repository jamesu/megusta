#if defined(_WIN32) && !defined(FD_SETSIZE)
#define FD_SETSIZE 256
#endif
#include "socket.h"
#ifdef _WIN32
// resource_runtime.h reaches directory_services.h and then windows.h. Include
// Winsock 2 first so windows.h does not declare the older winsock.h API.
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include "resource_runtime.h"
#include <cerrno>
#include <cstring>
#include <cstdlib>
#ifndef _WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace megusta::engine_socket {

namespace {
bool winsockStarted=false;
void networkServiceEnd() noexcept {} // recovered netc_serviceEnd_stub body is a single return
}

std::uint32_t engineNet_hostToNetwork32_adapter(std::uint32_t value) { return htonl(value); }
std::uint16_t engineNet_hostToNetwork16_adapter(std::uint16_t value) { return htons(value); }
std::uint32_t engineNet_networkToHost32_adapter(std::uint32_t value) { return ntohl(value); }

void engineNet_shutdownWinsock_impl() {
    networkServiceEnd();
#ifdef _WIN32
    const int result=::WSACleanup();
    // The recovered code compares against 0x2734 (WSAEINPROGRESS), then
    // clears the Winsock error and retries cleanup once.
    if(result==SOCKET_ERROR && ::WSAGetLastError()==0x2734) {
        ::WSASetLastError(0);
        (void)::WSACleanup();
    }
#endif
}

int engineNet_initializeWinsock_impl() {
#ifdef _WIN32
    if(!winsockStarted) {
        WSADATA data{};
        if(::WSAStartup(MAKEWORD(1,1),&data)!=0) {
            if (sharedIo && sharedIo->formatted && sharedIo->standardError)
                sharedIo->formatted(sharedIo->standardError,"Couldn't initialize Winsock 1.1 ");
            return -1;
        }
        winsockStarted=true;
        std::atexit(engineNet_shutdownWinsock_impl);
    }
#endif
    return 0;
}

void engineSocket_translateLastError_impl() {
#ifdef _WIN32
    const int error=WSAGetLastError();
    if(error!=0) {
        *_errno()=error-10000;
        WSASetLastError(0);
    }
#endif
}

int engineSocket_accept_adapter(int socket,void* address,int* addressLength) {
#ifdef _WIN32
    const int result=static_cast<int>(::accept(static_cast<SOCKET>(socket),
        static_cast<sockaddr*>(address),addressLength));
#else
    const int result=static_cast<int>(::accept(socket,static_cast<sockaddr*>(address),
        reinterpret_cast<socklen_t*>(addressLength)));
#endif
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_bind_adapter(int socket,const void* address,int addressLength) {
    const int result=::bind(socket,static_cast<const sockaddr*>(address),addressLength);
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_close_adapter(int socket) {
#ifdef _WIN32
    const int result=::closesocket(static_cast<SOCKET>(socket));
#else
    const int result=::close(socket);
#endif
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_connect_adapter(int socket,const void* address,int addressLength) {
    const int result=::connect(socket,static_cast<const sockaddr*>(address),addressLength);
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_listen_adapter(int socket,int backlog) {
    const int result=::listen(socket,backlog); engineSocket_translateLastError_impl(); return result;
}
int engineSocket_recv_adapter(int socket,char* buffer,int length,int flags) {
    const int result=static_cast<int>(::recv(socket,buffer,length,flags));
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_recvfrom_adapter(int socket,char* buffer,int length,int flags,void* address,int* addressLength) {
#ifdef _WIN32
    const int result=static_cast<int>(::recvfrom(socket,buffer,length,flags,
        static_cast<sockaddr*>(address),addressLength));
#else
    const int result=static_cast<int>(::recvfrom(socket,buffer,length,flags,
        static_cast<sockaddr*>(address),reinterpret_cast<socklen_t*>(addressLength)));
#endif
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_send_adapter(int socket,const char* buffer,int length,int flags) {
    const int result=static_cast<int>(::send(socket,buffer,length,flags));
    engineSocket_translateLastError_impl(); return result;
}
int engineSocket_sendto_adapter(int socket,const char* buffer,int length,int flags,const void* address) {
    const int result=static_cast<int>(::sendto(socket,buffer,length,flags,
        static_cast<const sockaddr*>(address),16));
    engineSocket_translateLastError_impl(); return result;
}
void engineSocket_shutdown_adapter(int socket,int how) { (void)::shutdown(socket,how); }
int engineSocket_create_adapter(int domain,int type,int protocol) {
    return static_cast<int>(::socket(domain,type,protocol));
}
std::uint32_t engineSocket_inetAddr_adapter(const char* address) {
    return static_cast<std::uint32_t>(::inet_addr(address));
}
char* engineSocket_inetNtoa_adapter(std::uint32_t address) {
    in_addr value{}; value.s_addr=address; return ::inet_ntoa(value);
}
int engineSocket_getsockoptUnsupported_impl() { return -1; }
int engineSocket_setOptionOrNonblocking_impl(int socket,int unused,int option,const int* value,int length) {
    (void)unused;
    if(option==0x20) {
        return ::setsockopt(socket,SOL_SOCKET,SO_BROADCAST,
            reinterpret_cast<const char*>(value),length);
    }
    if(option==0x200) {
        unsigned long nonblocking=static_cast<unsigned long>(*value);
#ifdef _WIN32
        return ::ioctlsocket(static_cast<SOCKET>(socket),FIONBIO,&nonblocking);
#else
        int current=::fcntl(socket,F_GETFL,0);
        if(current<0) return -1;
        return ::fcntl(socket,F_SETFL,*value ? (current|O_NONBLOCK) : (current&~O_NONBLOCK));
#endif
    }
    return -1;
}
std::uint16_t engineSocket_ntohs_adapter(std::uint16_t value) { return ntohs(value); }

int engineSocket_pollWithSelect_impl(PollEntry* entries,int count,int timeoutMilliseconds) {
    if(count<=0) return 0;
    fd_set readSet,writeSet;
    FD_ZERO(&readSet); FD_ZERO(&writeSet);
    bool wantRead=false,wantWrite=false;
    int requestedEventBits=0;
    int maxSocket=-1;
    for(int i=0;i<count;++i) {
        entries[i].returnedEvents=0;
        if(entries[i].events&pollRead) { FD_SET(entries[i].socket,&readSet); wantRead=true; ++requestedEventBits; }
        if(entries[i].events&pollWrite) { FD_SET(entries[i].socket,&writeSet); wantWrite=true; ++requestedEventBits; }
        if(entries[i].socket>maxSocket) maxSocket=entries[i].socket;
    }
    if(!requestedEventBits) return 0;
    timeval timeout{};
    timeout.tv_sec=timeoutMilliseconds/1000;
    timeout.tv_usec=(timeoutMilliseconds%1000)*1000;
#ifdef _WIN32
    (void)::select(0,wantRead?&readSet:nullptr,wantWrite?&writeSet:nullptr,nullptr,&timeout);
#else
    (void)::select(maxSocket+1,wantRead?&readSet:nullptr,wantWrite?&writeSet:nullptr,nullptr,&timeout);
#endif
    int eventCount=0;
    for(int i=0;i<count;++i) {
        if((entries[i].events&pollRead) && FD_ISSET(entries[i].socket,&readSet)) {
            entries[i].returnedEvents|=pollRead; ++eventCount;
        }
        if((entries[i].events&pollWrite) && FD_ISSET(entries[i].socket,&writeSet)) {
            entries[i].returnedEvents|=pollWrite; ++eventCount;
        }
    }
    return eventCount;
}

int engineSocket_gethostname_adapter(char* name,int length) { return ::gethostname(name,length); }
int engineSocket_getsockname_adapter(int socket,void* address,int* addressLength) {
#ifdef _WIN32
    return ::getsockname(socket,static_cast<sockaddr*>(address),addressLength);
#else
    return ::getsockname(socket,static_cast<sockaddr*>(address),reinterpret_cast<socklen_t*>(addressLength));
#endif
}
void* engineSocket_gethostbyaddr_adapter(const char* address,int length,int family) {
    return ::gethostbyaddr(address,length,family);
}
void* engineSocket_gethostbyname_adapter(const char* name) { return ::gethostbyname(name); }
void* engineSocket_getprotobyname_adapter(const char* name) { return ::getprotobyname(name); }
void* engineSocket_getservbyname_adapter(const char* name,const char* protocol) {
    return ::getservbyname(name,protocol);
}

} // namespace megusta::engine_socket
