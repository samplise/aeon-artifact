#include "SysUtil.h"
#include "lib/mace.h"

#include "TcpTransport-init.h"
#include "load_protocols.h"
#include <signal.h>
#include <string>
#include "mlist.h"

#include "RandomUtil.h"
#include <iostream>
#include <tagClientGUI.h>
#include "TagClientRGServiceClass.h"
#include "TagClientRGDataHandler.h"
#include "TagClientRG-init.h"
#include "ContextJobApplication.h"
 
using namespace std;
 
class TagClientRGResponseHandler : public TagClientRGDataHandler {
  void roomMapResponseReceived(uint16_t location, const mace::array<mace::array<int, 50>, 50> & roomMap) {
    std::cout << "Haha, I got a room map" << std::endl;
  }
};


int main(int argc, char* argv[]) {
  mace::Init(argc, argv);
  load_protocols();

  mace::string service = "TagClientRG";
  mace::ContextJobApplication<TagClientRGServiceClass> app;
  app.installSignalHandler();

  params::print(stdout);

  app.loadContext();

  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;
  app.startService( service );
  
  app.getServiceObject()->kidInit();
  /* Start the NCurses GUI part for Tag */
  initscr();
  printw("Welcome to Tag !!!");
  refresh();
  clear();

  int haha[10][10];

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      printw("%d ", haha[i][j]);
    }
    printw("\n");
  }
  printw("\nPress any key to continue\n");
  refresh();
  getch();
  endwin();

  app.globalExit();
  return 0;
} 
