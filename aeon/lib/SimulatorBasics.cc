#include "SimulatorBasics.h"

namespace macesim {

  bool SimulatorFlags::isSimulated = false;
  int SimulatorFlags::currentNode = 0;
  int SimulatorFlags::pathNum = -1;
  mace::string SimulatorFlags::currentNodeStr = "";

  TestPropertyList TestProperties::propertiesToTest;
}
