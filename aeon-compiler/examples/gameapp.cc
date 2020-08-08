actorclass Building {
	Room room;
	
	void initialize();
}

void Building::initialize() {
	ADD_SELECTORS("GameApp");
	maceout << "Create a Building." << Log::endl;
	room = createActor<Room>(1);
	room.initialize(4, 2);
}

actorclass Room {
	vector<Player> players;
	vector<Item> items;
	int nextPlayer;

	void initialize(int nPlayer, int nItem);
	void updatePlayer();
	void getPlayer(Client c);
}

void Room::initialize(int nPlayer, int nItem) {
	ADD_SELECTORS("GameApp");
	maceout << "Create a Room." << Log::endl;
	for(int i=0; i<nItem; i++) {
		Item item = createActor<Item>();
		items.push_back(item);
		item.initialize();
	}

	for(int i=0; i<nPlayer; i++) {
		Player p = createActor<Player>();
		players.push_back(p);

		int j = i % items.size();
		Item item = items[j];
		p.initialize(item);
	}

	nextPlayer = 0;
}

void Room::updatePlayer() {
	for(int i=0; i<players.size(); i++) {
		async players[i].update();
	}
}

void Room::getPlayer(Client c) {
	event c.updatePlayer(players[nextPlayer]);
	nextPlayer ++;
}

actorclass Player {
	Item item;
	int count;

	void initialize(Item i);
	void accessItem(Client c);
	void update();
}

void Player::initialize(Item i) {
	ADD_SELECTORS("GameApp");
	maceout << "Create a Player." << Log::endl;
	item = i;
	count = 0;
}

void Player::accessItem(Client c) {
	item.access();
	event c.reply();
}

void Player::update() {
	count = count + 1;
}

actorclass Item {
	int count;
	Client client;
	
	void initialize();
	void access();
}

void Item::initialize() {
	ADD_SELECTORS("GameApp");
	maceout << "Create a Item." << Log::endl;
	count = 0;
}

void Item::access() {
	count ++;
}

actorclass Client {
	int count;
	yield Player player;

	void initialize();
	void updatePlayer(Player p);
	void reply();
}

void Client::initialize() {
	ADD_SELECTORS("GameApp");
	maceout << "Create a Client." << Log::endl;
	sleep(3);
	count = 0;
	Room r = getActor<Room>(1);
	event r.getPlayer(this);
}

void Client::updatePlayer(Player p) {
	player = p;

	event player.accessItem(this);
}

void Client::reply() {
	ADD_SELECTORS("GameApp");
	count ++;
	if( count%1000 == 0) {
		maceout << "Client access the Player for " << count << Log::endl;
	}
	event player.accessItem(this);
}

void main() {
	Building b = createActor<Building>();
	event b.initialize();

	for(int i=0; i<4; i++) {
		int cID = i+1;
		Client c = createActor<Client>(cID);
		event c.initialize();
	}
}