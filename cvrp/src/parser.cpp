#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>

enum class Section {
  NONE,
  NODE_COORD,
  DEMAND,
  DEPOT
};

struct Node {
  int id;
  int x, y;
  int demanda;
};

struct VRPInstance {
  int depot_id; 
  int capacity;
  std::vector<Node> nodes;
  std::string name;
};

std::string trim(const std::string& line) {
    auto first = std::find_if(line.begin(), line.end(), [](unsigned char c) {
        return std::isalnum(c);
    });

    auto last = std::find_if(line.rbegin(), line.rend(), [](unsigned char c) {
        return std::isalnum(c);
    });

    if (first == line.end()) return "";

    size_t pos_first = std::distance(line.begin(), first);
    size_t pos_last  = std::distance(line.begin(), last.base());

    return line.substr(pos_first, pos_last - pos_first);
}

int main() {
    std::ifstream inFile;
    inFile.open("data/A-n32-k5.vrp");

    std::string line;

    if(!inFile) {
        std::cout << "Erro ao abrir o arquivo." << std::endl;
        return 1;
  }
 
  Section current_section = Section::NONE; 

  while(getline(inFile, line)) {
    std::cout << "[" << trim(line) << "]" << std::endl;
    if (line == "NODE_COORD_SECTION") {
      current_section = Section::NODE_COORD;
      continue;
    }
    else if (line == "DEMAND_SECTION") {
        current_section = Section::DEMAND;
        continue;
    }
    else if (line == "DEPOT_SECTION") {
        current_section = Section::DEPOT;
        continue;
    }
  }

    inFile.close();

    return 0;
}
