#minclude "TagItemShareAppMessages.mi"

constants {
  uint16_t DIRECTION_UP = 1;
  uint16_t DIRECTION_DOWN = 2;
  uint16_t DIRECTION_LEFT = 3;
  uint16_t DIRECTION_RIGHT = 4;
  
  uint16_t ROOM_SIZE = 100;
}
 
constructor_parameters {
  uint32_t N_BUILDINGS = 1;
  uint32_t N_ROOMS_PER_BUILDING = 10;
  uint32_t N_ITEMS_PER_ROOM = 10;
  uint32_t N_ITEMS_PER_PLAYER = 2;

  uint16_t MOVE_AROUND_BUILDING = 1;
  uint16_t DETAILED_FLAG = 0;

  uint32_t GEN_IMAGE_TIME_PERIOD = 0;

  uint16_t TO_SHARE_ITEM = 1;
}

typedefs {
  typedef mace::map<Coordinate, int> portalMap; 
}

struct Coordinate {
  int16_t x;
  int16_t y;
};

struct PlayerInfo {
  Coordinate coord;
  uint16_t playerDirection;
};

actorclass World {
  mace::vector<Building> buildingIds;
  uint32_t nextBuildingIter;

  void worldInit();
  void initPlayerInWorld( const uint32_t clientId );
};

actorcalss Building {
  uint32_t buildingId;
  mace::vector<Room> roomIds;
  uint32_t nextRoomIter;

  void initBuilding(uint32_t nBuilding);
  void initPlayerInBuilding( const Player nPlayer, const uint32_t clientId );
  void movePlayerAroundBuilding( const Room nRoom, const Player nPlayer, const uint32_t clientId, const MaceKey& src);
};

actorclass Room {
  uint32_t buildingId;
  uint32_t roomId;
  mace::set<Player> playersInRoom;
  mace::vector<Item> itemIds;

  void initRoom(uint32_t nRoom, uint32_t nBuilding);
  void initPlayerInRoom( const uint32_t nBuilding, const Player nPlayer, const uint32_t clientId );
  void MoveRequest( const uint32_t clientId, const Player playerId, const Room roomId );
  void changeItemInRoom( const Player nPlayer, const uint32_t& clientId, const uint64_t& reqId );
  void movePlayerFromRoom(const Player nPlayer);
};

actorclass Item {
  uint32_t itemId;
  uint32_t roomId;
  uint32_t quantity;

  void initItem(const uint32_t& nItem, uint32_t nRoom);
  void accessItem();
};

actorclass Player {
  uint32_t playerId;

  uint32_t curBuildingId;
  uint32_t curRoomId;
  Coordinate coord;

  uint16_t playerDirection;
  uint16_t directChangeCount;

  mace::set<Item> itemIds;

  void updatePlayerInfo( const uint32_t nBuilding, const uint32_t nRoom, const Coordinate& newCoord, const uint16_t new_direction );
  void ItemAccessRequest( const uint32_t clientId, const uint32_t reqId );
  void initPlayer( const uint32_t nPlayer, const uint32_t nRoom, const uint32_t nBuilding, const Coordinate& cd, 
    const mace::set<Item>& nItems );
  mace::set<Item> getItems();
  void changeItem( const Item& new_itemId );
  void preparePlayerMovement();
};

void World::worldInit() {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "Initilaize world!" << Log::endl;
    
  nextBuildingIter = 0;
  // Initialize the building entrance
  for (uint32_t i = 0; i < N_BUILDINGS; i++) {
    Building building = createNewActor<Building>();
    buildingIds.push_back(buildingId);      
  }

  for( uint32_t i=0; i<buildingIds.size(); i++ ){
      event buildingIds[i].initBuilding( buildingIds[i].getActorID() );
  }
}

void Building::initBuilding (uint32_t nBuilding) {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "Initialize Building " << nBuilding << Log::endl;
  ASSERT(nBuilding >= 1);
  buildingId = nBuilding; 
  nextRoomIter = 0;
    
  for (uint32_t rCount = 0; rCount < N_ROOMS_PER_BUILDING; rCount++) {
    Room room = createNewActor<Room>();
    roomIds.push_back(room);
  }

  for( uint32_t i=0; i<roomIds.size(); i++ ){
    event roomIds[i].initRoom(roomIds[i].getActorID(), nBuilding);
  }
}

void Room::initRoom(uint32_t nRoom, uint32_t nBuilding) {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "Initialize Building["<< nBuilding <<"]room["<< nRoom <<"]!" << Log::endl;
  ASSERT(nBuilding > 0 && nRoom > 0);
    
  for (uint32_t i = 0; i < ROOM_SIZE; i++) {
    for (uint32_t j = 0; j < ROOM_SIZE; j++) {
      roomMap[i][j] = -1;
    }
  }
  roomId = nRoom;
  buildingId = nBuilding;

  for (uint32_t i = 0; i < N_ITEMS_PER_ROOM; i++) {
    Item item = createNewActor<Item>();
    itemIds.push_back(item);
    event itemIds[i].initItem(itemIds[i].getActorID(), nRoom);
  }
}

void Item::initItem(const uint32_t& nItem, uint32_t nRoom) {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "Initialize Room["<< nRoom <<"]Item["<< nItem <<"]!" << Log::endl;
    
  itemId = nItem;
  roomId = nRoom;
  quantity = 0;
}

void World::initPlayerInWorld( const uint32_t clientId ) {
  ADD_SELECTORS("TagItemShareAppServer");

  Player newPlayer = createNewContext("Player");
  maceout << "Assign player("<< newPlayer.getActorID() <<") to client("<< clientId<<") from " << src << Log::endl;
    
  Building nBuilding = buildingIds[ nextBuildingIter ];
  nextBuildingIter = (nextBuildingIter+1) % buildingIds.size();
  async nBuilding.initPlayerInBuilding( newPlayer, clientId );
}

void Building::initPlayerInBuilding( const Player nPlayer, const uint32_t clientId ) {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "To initialze Player("<< nPlayer.getActorID() <<") in Building["<< buildingId <<"]!" << Log::endl;

  //uint32_t nRoomIter = RandomUtil::randInt() % roomIds.size();
  uint32_t nRoomIter = nextRoomIter;
  nextRoomIter = (nextRoomIter+1) % roomIds.size();

  Room nRoom = roomIds[nRoomIter];
  async nRoom.initPlayerInRoom( buildingId, nPlayer, clientId );
}

void Room::initPlayerInRoom( const uint32_t nBuilding, const Player nPlayer, const uint32_t clientId ) {
  ADD_SELECTORS("TagItemShareAppServer");
  maceout << "To initialze Player("<< nPlayer.getActorID() <<") in Building["<< nBuilding <<"]Room["<< roomId <<"]!" << Log::endl;

  mace::set<Item> nItems;

  for( uint32_t i=0; i<N_ITEMS_PER_PLAYER; i++ ){
    while(true) {
      uint32_t nItemIter = RandomUtil::randInt() % itemIds.size();
      Item nItem = itemIds[nItemIter];
      if( nItems.count(nItem) == 0 ){
        nItems.insert(nItem);
        break;
      }
    }
  }
    
  Coordinate cd;
  while( true ){
    cd.x = RandomUtil::randInt(ROOM_SIZE);
    cd.y = RandomUtil::randInt(ROOM_SIZE);

    if( cd.x == 0 && cd.y == 0 ){
      continue;
    }

    if( roomMap[cd.x][cd.y] >= 0 ){
      continue;
    }

    roomMap[cd.x][cd.y] = nPlayer;
    break;
  }

  playersInRoom.insert(nPlayer);
  nPlayer.initPlayer( roomId, nBuilding, cd, nItems );
  reply( src, PlayerInitReply(clientId, nPlayer.getActorID(), roomId, nBuilding) );
}

void Room::MoveRequest( const uint32_t clientId, const Player playerId, const Room roomId ) {
    ADD_SELECTORS("TagAppItemShareServer");
    ASSERT( playersInRoom.count(playerId) > 0 );

    PlayerInfo playerInfo = playerId.getPlayerInfo();
    
    Coordinate newCoord;
    newCoord.x = playerInfo.coord.x;
    newCoord.y = playerInfo.coord.y;
    
    switch (playerInfo.playerDirection) {
      case DIRECTION_UP:
        newCoord.y++;
        break;
      case DIRECTION_DOWN:
        newCoord.y--;
        break;
      case DIRECTION_LEFT:
        newCoord.x--;
        break;
      case DIRECTION_RIGHT:
        newCoord.x++;
        break;
      default:
        ABORT("Player direction invalid!");
    }

    if( newCoord.x<0 || newCoord.x >= ROOM_SIZE || newCoord.y<0 || newCoord.y>=ROOM_SIZE || roomMap[newCoord.x][newCoord.y]!=-1 ) {
      uint16_t new_direction = playerInfo.playerDirection + 1;
      if( new_direction>4 ) new_direction = 1;
      async playerId.updatePlayerInfo( buildingId, roomId, playerInfo.coord, new_direction );
      reply( src, RequestReply(msg.clientId, buildingId, roomId, 0) );
    } else {
      roomMap[newCoord.x][newCoord.y] = msg.playerId;
      roomMap[playerInfo.coord.x][playerInfo.coord.y] = -1;

      if( newCoord.x==0 && newCoord.y == 0 && MOVE_AROUND_BUILDING == 1 ) {
        Building building = getActor<Building>(buildingId);
        event building.movePlayerAroundBuilding(roomId, playerId, clientId, src);
      } else {
        async playerId.updatePlayerInfo( buildingId, roomId, newCoord, playerInfo.playerDirection );
        reply( src, RequestReply(clientId, buildingId, roomId, 0) );
      }
    }
  }

void Player::updatePlayerInfo( const uint32_t nBuilding, const uint32_t nRoom, const Coordinate& newCoord, 
    const uint16_t new_direction ) {
  curBuilding = nBuilding;
  curRoom = nRoom;
  coord.x = newCoord.x;
  coord.y = newCoord.y;
  playerDirection = new_direction;
}

void Building::movePlayerAroundBuilding( const Room nRoom, const Player nPlayer, const uint32_t clientId, const MaceKey& src) {
  ADD_SELECTORS("TagAppItemShareServer");
  nRoom.movePlayerFromRoom(nPlayer);

  Room new_nRoom = nRoom;
  while( new_nRoom == nRoom ){
    uint32_t nRoomIter = RandomUtil::randInt() % roomIds.size();
    new_nRoom = roomIds[nRoomIter];
  }

  new_room.movePlayerToRoom( buildingId, nPlayer);
  reply( src, RequestReply(clientId, buildingId, new_nRoom, 0) );
}

void Player::ItemAccessRequest( const uint32_t clientId, const uint32_t reqId ) {
  ADD_SELECTORS("TagItemShareAppServer");
    
  for( mace::set<Item>::iterator iter=itemIds.begin(); iter!=itemIds.end(); iter++ ){
    async (*iter).accessItem();
  }
  reply( src, RequestReply( clientId, 0, 0, reqId) );
}

void Item::accessItem(){
  quantity ++;
}

void Room::changeItemInRoom( const Player nPlayer, const uint32_t& clientId, const uint64_t& reqId ) {
  mace::set<Item> p_itemIds = nPlayer.getPlayerItems();
  Item new_itemId = NULL;

  uint32_t endIter = RandomUtil::randInt() % itemIds.size();
  uint32_t startIter = (endIter + 1) % itemIds.size();
  uint32_t i = startIter;
  while( i != endIter ){
    if( p_itemIds.count(itemIds[i]) == 0 ) {
      new_itemId = itemIds[i];
      break; 
    }
    i = (i + 1) % itemIds.size();
  }

  if( new_itemId != NULL ) {
    nPlayer.changeItem( new_itemId );
  }

  reply( src, RequestReply(clientId, 0, 0, reqId) );  
}

void Player::initPlayer( const uint32_t nPlayer, const uint32_t nRoom, const uint32_t nBuilding, const Coordinate& cd, 
    const mace::set<Item>& nItems ) {
  ADD_SELECTORS("TagItemShareAppServer");
      
  playerId = nPlayer;
  curBuilding = nBuilding;
  curRoom = nRoom;
           
  coord.x = cd.x;
  coord.y = cd.y;
  playerDirection = RandomUtil::randInt(4)+1;
  itemIds = nItems;
}

mace::set<Item> Player::getItems() {
  ADD_SELECTORS("TagItemShareAppServer");
  return itemIds;
}

void Player::changeItem( const Item& new_itemId ) {
  if( itemIds.size() > 0 ) {
    mace::set<Item>::iterator iter=itemIds.begin();
    Item nItem = *iter;
    itemIds.erase( nItem );
  }

  if( new_itemId != NULL && itemIds.count(new_itemId) == 0 ) {
    itemIds.insert(new_itemId);
  }
}

void Player::preparePlayerMovement() {
  ADD_SELECTORS("TagItemShareAppServer");
  coord.x = 0;
  coord.y = 0;

  itemIds.clear();
}

void Room::movePlayerFromRoom(const Player nPlayer) {
  ADD_SELECTORS("TagItemShareAppServer");
  ASSERT( playersInRoom.count(nPlayer) > 0 );
  nPlayer.preparePlayerMovement();

  playersInRoom.erase(nPlayer);
}
