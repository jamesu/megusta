// Local-only NET integration. No external peers, room servers or game discovery.
#include "netMessage.h"
#include "netChat.h"
#include "netRoom.h"
#include <array>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <algorithm>

static unsigned failures=0;
static void check(bool ok,const char* name){if(!ok){++failures;std::fprintf(stderr,"FAIL %s\n",name);}}
class RoomClient : public netRoomClient {
public:
    unsigned entered=0,joined=0,left=0;
    std::vector<std::vector<char>> sent;
    RoomClient(const netGuid& guid):netRoomClient(guid,28000){}
    bool bufferSend(const char* bytes,int n)override{sent.emplace_back(bytes,bytes+n);return true;}
    void handleEnterRoom()override{++entered;}
    void handleJoinGame(int)override{++joined;}
    void handleLeaveGame(int)override{++left;}
};
static void rooms() {
    netGuid guid{};RoomClient client(guid);netRoom room("127.0.0.1",9,123);
    client.enterRoom("tester",room,nullptr);
    check(client.sent.size()==1,"room login queued");
    netMessage login(client.sent[0].data(),int(client.sent[0].size()));
    check(login.getType()==200 && login.geti()==0x22,"fixed room wire version");
    netMessage reply(210,9);reply.puti(0);netMessage incoming(reply.getData(),reply.getLength());client.handleMessage(incoming);
    check(client.entered==1 && client.getLocalID()==9,"successful login callback");
    netMessage game(213,9);game.puti(73);game.puts("sparse");game.puti(9);game.puti(4);game.puti(0x12345678);game.putb(false);game.putb(true);
    netMessage update(game.getData(),game.getLength());client.handleMessage(update);
    check(client.getGame(73) && !client.getGame(1) && client.getGame(73)->protocolValue==0x12345678,"incoming sparse game id and extra word");
    netMessage player(217,9);player.puti(9);player.puts("tester");player.puti(73);player.putb(false);player.puti(0xabcdef01);
    netMessage change(player.getData(),player.getLength());client.handleMessage(change);
    check(client.getPlayer(9) && client.getPlayer(9)->protocolValue==0xabcdef01 && client.inGame() && client.isGameMaster() && client.joined==1,"sparse player identity and game callback");
    client.startGame();check(client.sent.back()[2]==char(205),"master start opcode");
    client.createGame("new","pass",4);check(client.sent.back()[2]==char(202),"create opcode after leave");
    netMessage okay(210,9);okay.puti(0);netMessage ready(okay.getData(),okay.getLength());client.handleMessage(ready);
    client.joinGame("sparse",nullptr);check(client.sent.back()[2]==char(203),"join opcode");
    netMessage again(okay.getData(),okay.getLength());client.handleMessage(again);
    client.watchGame("sparse",nullptr);netMessage watching(client.sent.back().data(),int(client.sent.back().size()));
    check(watching.geti()==73 && watching.getb(),"watch flag and game id");
    client.leaveRoom();check(client.sent.back()[2]==char(201) && !client.getGame(73) && !client.getPlayer(9),"room close deletes records");
}
class MessageSink : public netMessageChannel {
public:
    std::vector<std::vector<char>> messages;
    void handleMessage(const netMessage& msg)override {messages.emplace_back(msg.getData(),msg.getData()+msg.getLength());}
    void feed(netBuffer& b){static_cast<netBufferChannel&>(*this).handleBufferRead(b);}
};
class ChatSink : public netChat {
    std::string current;
public:
    std::vector<std::string> lines;
    ~ChatSink()override{std::free(const_cast<char*>(getTerminator()));} // Test owner cleans the retained stock terminator allocation.
    void collectIncomingData(const char* s,int n)override {current.append(s,n);}
    void foundTerminator()override {lines.push_back(current);current.clear();}
    void feed(netBuffer& b){static_cast<netBufferChannel&>(*this).handleBufferRead(b);}
};
class Channel : public netChannel {
public:
    unsigned opens=0,closes=0,writes=0;bool wasConnectedAtClose=false;
    std::string received;
    void handleOpen()override{++opens;}
    void handleClose()override{++closes;wasConnectedAtClose=isConnected();}
    void handleWrite()override{++writes;}
    void handleRead()override{char data[64];int n=recv(data,sizeof(data));if(n>0)received.append(data,n);}
};
class BufferChannel : public netBufferChannel {
public:
    unsigned closes=0;
    BufferChannel():netBufferChannel(8,8){}
    void handleClose()override{++closes;netBufferChannel::handleClose();}
};
static int port_of(netSocket& socket) {
    netAddress address;int size=sizeof(address);
    if(::getsockname(socket.getHandle(),reinterpret_cast<sockaddr*>(&address),&size))throw std::runtime_error("getsockname failed");
    return address.getPort();
}
static void fatal_callback(ulSeverity severity,char*){if(severity==UL_FATAL)throw std::runtime_error("expected framing rejection");}
static void buffers() {
    netBuffer b(8);check(b.append("12345678",8),"plain buffer accepts exact capacity");
    check(!b.append("x",1) && b.getLength()==8,"plain buffer rejects overflow");
    b.remove(2,3);check(std::string(b.getData())=="12678","buffer overlap removal");b.remove();
    BufferChannel channel;check(channel.open(),"buffer channel open");
    check(channel.bufferSend("1234567",7),"channel accepts below capacity");
    check(!channel.bufferSend("8",1) && channel.isClosed() && channel.closes==1,"channel exact capacity closes");
    check(!static_cast<netChannel&>(channel).writable(),"close clears pending output");
    channel.close();check(channel.closes==1 && !channel.isConnected(),"repeated close flags and callback count");
}
static void messages() {
    netMessage m(0x42,7,9,128);m.putw(0x1234);m.puti(0x01020304);m.putf(1.5f);m.puts("hi");
    uint32_t bits[]={0x80000000u,0x7fc12345u};float values[2];std::memcpy(values,bits,8);m.putfv(values,2);
    const unsigned char expected[]={0,26,0x42,7,9,0x12,0x34,1,2,3,4,0x3f,0xc0,0,0,'h','i',0,0x80,0,0,0,0x7f,0xc1,0x23,0x45};
    check(m.getLength()==sizeof(expected) && !std::memcmp(m.getData(),expected,sizeof(expected)),"golden network-order bytes");
    netMessage incoming(m.getData(),m.getLength());check(incoming.getw()==0x1234 && incoming.geti()==0x01020304 && incoming.getf()==1.5f,"integer/float decoding");
    char truncated[2];incoming.gets(truncated,sizeof(truncated));check(std::string(truncated)=="h","bounded string consumes entire field");
    float read[2];incoming.getfv(read,2);check(!std::memcmp(read,bits,8),"signed-zero/NaN payload word preservation");
    netMessage full(1,2,3,8);full.puta("abcdef",6);check(full.getLength()==5 && full.getData()[1]==5,"failed append retains frame length");
    netMessage other(11,1);other.puti(42);
    std::vector<char> stream(m.getData(),m.getData()+m.getLength());stream.insert(stream.end(),other.getData(),other.getData()+other.getLength());
    MessageSink sink;netBuffer input(512);size_t offset=0;
    for(size_t count:{1u,1u,3u,2u,100u}) {
        count=std::min(count,stream.size()-offset);input.append(stream.data()+offset,int(count));offset+=count;sink.feed(input);
    }
    check(sink.messages.size()==2 && input.getLength()==0,"fragmented and pipelined frames");
    check(sink.messages.size()==2 && sink.messages[0]==std::vector<char>(m.getData(),m.getData()+m.getLength()),"callback frame bytes");
    auto previous=ulGetErrorCallback();ulSetErrorCallback(fatal_callback);
    bool rejected=false;
    try{netBuffer bad(8);const char short_frame[]={0,4,1,2};bad.append(short_frame,4);sink.feed(bad);}catch(const std::runtime_error&){rejected=true;}
    ulSetErrorCallback(previous);check(rejected,"short frame rejected without zero-length loop");
    ChatSink chat;chat.setTerminator("\r\n");netBuffer text(128);
    for(const char* piece:{"a\r","\nb\r\nc\r","\n"}){text.append(piece,int(std::strlen(piece)));chat.feed(text);}
    check(chat.lines==std::vector<std::string>({"a","b","c"}) && text.getLength()==0,"split chat terminator");
}
static void sockets() {
    netAddress address("127.0.0.1",12345);check(std::string(address.getHost())=="127.0.0.1" && address.getPort()==12345,"address/port conversion");
    netAddress broadcast("<broadcast>",99);check(std::string(broadcast.getHost())=="255.255.255.255","broadcast address conversion only");
    WSASetLastError(WSAEWOULDBLOCK);check(netSocket::isNonBlockingError() && WSAGetLastError()==WSAEWOULDBLOCK,"nonblocking error retained");
    WSASetLastError(WSAECONNREFUSED);check(!netSocket::isNonBlockingError() && WSAGetLastError()==WSAECONNREFUSED,"other socket error retained");
    netSocket receiver,sender;
    if(!receiver.open(false) || receiver.bind("127.0.0.1",0) || !sender.open(false))throw std::runtime_error("Loopback UDP setup failed");
    netAddress destination("127.0.0.1",port_of(receiver));const char payload[]="localhost";
    check(sender.sendto(payload,sizeof(payload),0,&destination)==sizeof(payload),"UDP send");
    netSocket* reads[]={&receiver,nullptr};netSocket* writes[]={nullptr};
    check(netSocket::select(reads,writes,1000)==1 && reads[0]==&receiver,"UDP select");
    netSocket closed ;
    netSocket* duplicates[]={&receiver,&receiver,&closed,nullptr};netSocket* none[]={nullptr};
    check(netSocket::select(duplicates,none,0)==1 && duplicates[0]==&receiver && duplicates[1]==&receiver && duplicates[2]==nullptr,"duplicate readiness count and closed handle filtering");
    char data[64]{};netAddress from;
    check(receiver.recvfrom(data,sizeof(data),0,&from)==sizeof(payload) && !std::memcmp(data,payload,sizeof(payload)),"UDP receive");
    netSocket listener;
    if(!listener.open() || listener.bind("127.0.0.1",0) || listener.listen(1))throw std::runtime_error("Loopback TCP setup failed");
    Channel client,server;check(client.open() && client.opens==1,"channel open hook");
    check(client.connect("127.0.0.1",port_of(listener))==0,"nonblocking connect");
    netSocket* pending[]={&listener,nullptr};netSocket* no_writes[]={nullptr};
    check(netSocket::select(pending,no_writes,1000)>0,"TCP accept readiness");
    netAddress peer;int handle=listener.accept(&peer);if(handle==-1)throw std::runtime_error("TCP accept failed");
    server.setHandle(handle,true);check(server.opens==1 && server.isConnected(),"adopted-handle hook");
    for(int i=0;i<20 && !client.isConnected();++i)client.poll(10);
    check(client.isConnected(),"connect poll completion");
    check(client.send(payload,sizeof(payload))==sizeof(payload),"TCP send");
    for(int i=0;i<20 && server.received.empty();++i)client.poll(10);
    check(server.received==std::string(payload,sizeof(payload)),"TCP poll/read");
    client.close();check(client.closes==1 && client.wasConnectedAtClose && !client.isConnected(),"close callback ordering");
    for(int i=0;i<20 && !server.isClosed();++i)client.poll(10);
    check(server.isClosed() && server.closes==1,"EOF close path");
}
int main() {
    try {
        int argc=0;if(netInit(&argc,nullptr) || netInit(&argc,nullptr))throw std::runtime_error("Winsock init failed");
        buffers();messages();sockets();rooms();
        std::printf("PLIB NET transport/messages: %u failures; loopback only\n",failures);return failures?1:0;
    }catch(const std::exception& e){std::fprintf(stderr,"NET test error: %s\n",e.what());return 1;}
}
