/*
     PLIB - A Suite of Portable Game Libraries
     Copyright (C) 2001  Steve Baker
 
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
 
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
 
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the Free
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 
     For further information visit http://plib.sourceforge.net

     $Id: netRoomClient.cxx,v 1.4 2001/04/12 03:35:05 deca Exp $
*/

#include "netRoom.h"
#include <cstdio>
#include <cstring>

// MG-PLIB-061: original0046e750 fixed protocol text table. Small system
// errors use the target CRT service; unknown codes use the original format.
const char *netErrorString(int error)
{
  static const char *messages[]={
    "Too many games.","Too many rooms.","Invalid game.","Game is full.",
    "Game in progress.","Wrong game password","Wrong server password",
    "You have an old build; please download the latest version.","WRONG GUID",
    "Wrong nickname password","That password is illegal","That nickname is illegal",
    "That nickname is taken","That nickname does not exist; please register a new one.","INVALID MESSAGE"
  };
  static char unknown[128];
  if(error==0)return "";
  if(error==61)return "Could not find server; please try again later.";
  if(error>=100 && error<=114)return messages[error-100];
  if(error<100){const char *s=std::strerror(error);if(s)return s;}
  std::snprintf(unknown,sizeof(unknown),"Unknown Error (%d)",error);return unknown;
}
const char *netRoomClient::getErrorString()
{
  int error=last_error;
  if(state>STATE_DISCONNECT && state<STATE_READY && error)state=STATE_READY;
  last_error=0;
  return netErrorString(error);
}


netRoomClient::netRoomClient ( const netGuid& _guid, int _version ) : netMessageChannel(4096,8192)
{
  memcpy ( &guid, &_guid, sizeof(guid) ) ;
  version = 0x22 ; // MG-PLIB-013: fixed wire protocol, constructor argument is UDP port.
  discovery_port = (unsigned short)_version ;
  current_room = NULL ;
  discovery_socket = NULL ;
  state = STATE_DISCONNECT;
  last_error = 0 ;
}


netRoomClient::~netRoomClient ()
{
  leaveRoom () ;
  releaseDiscoverySocket () ;
}


void netRoomClient::enterRoom ( cchar* player_name, const netRoom& room,
  cchar* room_password )
{
  if ( ! current_room ) current_room = handleNewRoom () ;
  current_room->copy ( &room ) ;
  local_id = 0;
  state = STATE_DISCONNECT;
  last_error = 0 ;
  releaseDiscoverySocket () ; // MG-PLIB-028: after local login state reset, as at 00468770.

  open () ;
	connect ( room.host, room.port );

  if ( room_password == NULL )
    room_password = "";

  netMessage msg ( netRoom::SYSMSG_ENTER_ROOM, 0 ) ;
  msg.puti ( version ) ;
  msg.puta ( &guid, sizeof(guid) ) ;
  msg.puts ( player_name ) ;
  msg.puts ( room_password ) ;
  sendMessage ( msg ) ;

  state = STATE_LOGIN_REPLY;
}


void netRoomClient::leaveRoom ()
{
  if ( state != STATE_DISCONNECT )
  {
    netMessage msg ( netRoom::SYSMSG_LEAVE_ROOM, 0 ) ;
    sendMessage ( msg ) ;
    closeWhenDone() ;

    state = STATE_DISCONNECT;
    players.removeAll () ;
    games.removeAll () ;
    delete current_room ;
    current_room = NULL ;
  }
}
  

void netRoomClient::processUpdatePlayer ( const netMessage& msg )
{
  netRoomPlayer player ;
  player.get ( msg ) ;

  netRoomPlayer* p = getPlayer ( player.getID () ) ;
  if ( p == NULL )
    p = (netRoomPlayer*) players.addAt ( player.getID (), handleNewPlayer () ) ;
  if ( p != NULL )
  {
    int old_game = p -> game_id ;
    p -> copy ( &player ) ;
    
    //get local player
    netRoomPlayer* localp = getPlayer ( local_id ) ;

    //is game changing?
    if ( localp && old_game != p -> game_id )
    {
      //leave?
      if ( old_game )
      {
        //did player leave our game?
        if ( localp -> getID () == p -> getID () ||
          localp -> game_id == old_game )
          handleLeaveGame ( p -> getID () ) ;
      }
      else //join
      {
        //did player join our game?
        if ( localp -> game_id == p -> game_id )
          handleJoinGame ( p -> getID () ) ;
      }
    }
  }
}


void netRoomClient::processUpdateGame ( const netMessage& msg )
{
  netRoomGame game ;
  game.get ( msg ) ;
  
  netRoomGame* g = getGame ( game.getID () ) ;
  if ( g == NULL )
    g = (netRoomGame*) games.addAt ( game.getID (), handleNewGame () ) ;
  if ( g != NULL )
  {
    g -> copy ( &game ) ;

#if 0
    int old_master = g -> master_id ;
    //is master changing?
    if ( old_master != g -> master_id )
    {
      //get local player
      netRoomPlayer* localp = getPlayer ( local_id ) ;
      if ( localp && g -> getID () == localp -> game_id )
        handleSetGameMaster () ;
    }
#endif
  }
}

void netRoomClient::handleMessage ( const netMessage& msg )
{
  if ( state == STATE_DISCONNECT )
    return;

  switch ( msg.getType() )
  {
    case netRoom::SYSMSG_ERROR_REPLY:
      {
        last_error = msg.geti () ;
        switch (state)
        {
        case STATE_LOGIN_REPLY:
          if ( last_error == 0 )
          {
            local_id = msg.getToID();
            state = STATE_READY;
            handleEnterRoom () ;
          }
          break;
        case STATE_JOIN_REPLY:
          if ( last_error == 0 )
          {
            state = STATE_READY;
          }
          break;
        }
        break;
      }
    case netRoom::SYSMSG_REMOVE_ALL_GAMES:
      {
        games.removeAll () ;
        break;
      }
    case netRoom::SYSMSG_REMOVE_GAME:
      {
        int id = msg.geti () ;
        games.remove ( id ) ;
        break;
      }
    case netRoom::SYSMSG_UPDATE_GAME:
      {
        processUpdateGame ( msg ) ;
        break;
      }
    case netRoom::SYSMSG_REMOVE_ALL_PLAYERS:
      {
        players.removeAll () ;
        break;
      }
    case netRoom::SYSMSG_REMOVE_PLAYER:
      {
        int id = msg.geti () ;
        players.remove ( id ) ;
        break;
      }
    case netRoom::SYSMSG_UPDATE_PLAYER:
      {
        processUpdatePlayer ( msg ) ;
        break;
      }
    default:
      {
        netMessageChannel::handleMessage ( msg ) ;
        break;
      }
  }
}


void netRoomClient::createGame ( cchar* game_name, cchar* _password, 
    int max_players )
{
  leaveGame () ;

  if ( state == STATE_READY )
  {
    printf ( "Net: Creating game %s...\n", game_name ) ;

    char name[ NET_MAX_NAME+1 ];
    netCopyName ( name, game_name ) ;
  
    char password[ NET_MAX_NAME+1 ];
    netCopyName ( password, _password ) ;

    netMessage msg ( netRoom::SYSMSG_CREATE_GAME, 0 );
    msg.puti( max_players ) ;
    msg.puts( name );
    msg.puts( password );
    sendMessage ( msg ) ;

    //wait for reply
    state = STATE_JOIN_REPLY ;
    last_error = 0 ;
  }
}


void netRoomClient::joinGame ( cchar* game_name, cchar* _password )
{
  leaveGame () ;

  if ( state == STATE_READY )
  {
    const netRoomGame* found = NULL ;
    for ( int id = 1; id <= games . getNum (); id ++ )
    {
      const netRoomGame* g = getGame ( id ) ;
      if ( g && strcmp ( g -> name, game_name ) == 0 )
      {
        found = g ;
        break ;
      }
    }
  
    if ( found != NULL )
    {
      printf ( "Net: Joining game %s...\n", found -> name ) ;

      char password[ NET_MAX_NAME+1 ];
      netCopyName ( password, _password ) ;

      bool watching = false;
  
      netMessage msg( netRoom::SYSMSG_JOIN_GAME, 0 );
      msg.puti ( found -> getID () ) ;
      msg.putb ( watching ) ;
      msg.puts( password );
      sendMessage ( msg ) ;
  
      //wait for reply
      state = STATE_JOIN_REPLY ;
      last_error = 0 ;
    }
  }
}


void netRoomClient::watchGame ( cchar* game_name, cchar* _password )
{
  leaveGame () ;

  if ( state == STATE_READY )
  {
    const netRoomGame* found = NULL ;
    for ( int id = 1; id <= games . getNum (); id ++ )
    {
      const netRoomGame* g = getGame ( id ) ;
      if ( g && strcmp ( g -> name, game_name ) == 0 )
      {
        found = g ;
        break ;
      }
    }
  
    if ( found != NULL )
    {
      printf ( "Net: Watching game %s...\n", found -> name ) ;

      char password[ NET_MAX_NAME+1 ];
      netCopyName ( password, _password ) ;
  
      bool watching = true;
  
      netMessage msg( netRoom::SYSMSG_JOIN_GAME, 0 );
      msg.puti ( found -> getID () ) ;
      msg.putb ( watching ) ;
      msg.puts ( password ) ;
      sendMessage ( msg ) ;
  
      //wait for reply
      state = STATE_JOIN_REPLY ;
      last_error = 0 ;
    }
  }
}


void netRoomClient::leaveGame ()
{
  if ( inGame () && state == STATE_READY )
  {
    printf ( "Net: Leaving game...\n" ) ;

    netMessage msg( netRoom::SYSMSG_LEAVE_GAME, 0 );
    sendMessage ( msg ) ;
  }
}


void netRoomClient::startGame ()
{
  netRoomPlayer* p = getPlayer ( local_id ) ;
  if ( p )
  {
    const netRoomGame* g = getGame ( p -> game_id ) ;
    if ( g && g -> master_id == local_id )
    {
      printf ( "Net: Starting game...\n" ) ;

      netMessage	msg( netRoom::SYSMSG_START_GAME, 0 );
      sendMessage ( msg ) ;
    }
  }
}


bool netRoomClient::inGame () const
{
  const netRoomPlayer* p = ((netRoomClient*)this) -> getPlayer ( local_id ) ;
  return p && p -> game_id != 0 ;
}


bool netRoomClient::isGameMaster () const
{
  const netRoomPlayer* p = ((netRoomClient*)this) -> getPlayer ( local_id ) ;
  if ( p )
  {
    const netRoomGame* g = ((netRoomClient*)this) -> getGame ( p -> game_id ) ;
    return g && g -> master_id == local_id ;
  }
  return false ;
}


int netRoomClient::getGameMaster () const
{
  const netRoomPlayer* p = ((netRoomClient*)this) -> getPlayer ( local_id ) ;
  if ( p )
  {
    const netRoomGame* g = ((netRoomClient*)this) -> getGame ( p -> game_id ) ;
    if ( g )
      return g -> master_id ;
  }
  return 0 ;
}


void netRoomClient::setGameMaster ( int player_id )
{
  netRoomPlayer* p = getPlayer ( local_id ) ;
  if ( p )
  {
    const netRoomGame* g = getGame ( p -> game_id ) ;
    if ( g && ( g -> master_id == 0 || g -> master_id == local_id ) )
    {
      netRoomPlayer* m = getPlayer ( player_id ) ;
      if ( m && m -> game_id == p -> game_id && m -> getID () != g -> master_id )
      {
        printf ( "Net: Setting game master to %s...\n", m -> name ) ;
  
        netMessage	msg( netRoom::SYSMSG_SET_GAME_MASTER, 0 );
        msg.puti ( player_id ) ;
        sendMessage ( msg ) ;
      }
    }
  }
}

// MG-PLIB-013: binary 004682d0/004683e0/00468410. Analyst names.
bool netRoomClient::openDiscoverySocket ()
{
  if ( discovery_socket ) return true ;
  discovery_socket = new netSocket ;
  if ( discovery_socket->open ( false ) )
  {
    const char* host = netAddress::getLocalHost () ;
    int port = (short)discovery_port ;
    const int end = port + 25 ;
    while ( port < end && discovery_socket->bind ( host, port ) == -1 ) ++port ;
    if ( port < end ) { discovery_socket->setBlocking(false) ; return true ; }
  }
  printf ( "Net: failed to create udp socket, netRoomClient is cut off\n" ) ;
  releaseDiscoverySocket () ;
  return false ;
}
void netRoomClient::releaseDiscoverySocket ()
{
  if ( discovery_socket ) { discovery_socket->close() ; delete discovery_socket ; discovery_socket=NULL ; }
}
bool netRoomClient::poll ( unsigned int timeout )
{
  bool result = netChannel::poll ( timeout ) ;
  if ( discovery_socket )
  {
    char packet[256] ; netAddress from ;
    int length = discovery_socket->recvfrom ( packet, sizeof(packet), 0, &from ) ;
    if ( length > 0 )
    {
      netMessage message ( packet, length ) ;
      if ( message.getType() == 218 )
      {
        printf("Net: netRoom::SYSMSG_ROOM_INFO from = %s : %d\n",from.getHost(),from.getPort());
        handleRoomInfo ( message, from ) ;
      }
    }
  }
  return result ;
}

// MG-PLIB-048: original00468560/00468640; role names are inferred.
const netRoom* netRoomClient::getInfo ()
{
  if(!inRoom() || !current_room)return NULL;
  current_room->num_games=0;current_room->num_players=0;current_room->num_openings=0;
  for(int id=1;id<=255;++id)
    if(netRoomGame* game=getGame(id))
    {
      ++current_room->num_games;
      if(!game->in_progress)current_room->num_openings+=game->max_players;
    }
  for(int id=1;id<=255;++id)
    if(netRoomPlayer* player=getPlayer(id))
    {
      ++current_room->num_players;
      if(player->game_id)
        if(netRoomGame* game=getGame(player->game_id))
          if(!game->in_progress)--current_room->num_openings;
    }
  return current_room;
}
void netRoomClient::queryRoomInfo ( const netRoom& room )
{
  if(!discovery_socket)return;
  netAddress address(room.host,room.port);
  netMessage message(207,0);
  discovery_socket->sendto(message.getData(),message.getLength(),0,&address);
}
