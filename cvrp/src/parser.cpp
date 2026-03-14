#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>

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

  VRPInstance instance; 

  while(getline(inFile, line)) {
    std::cout << "[" << trim(line) << "]" << std::endl;

    if (trim(line) == "NODE_COORD_SECTION") {
      current_section = Section::NODE_COORD;  
      continue;
    }
    else if (trim(line) == "DEMAND_SECTION") {
      current_section = Section::DEMAND;
      continue;
    }
    else if (trim(line) == "DEPOT_SECTION") {
      current_section = Section::DEPOT;
      continue;
    }

    if (current_section == Section::NODE_COORD) {
      std::istringstream ss(line);

      Node node;    
      ss >> node.id >> node.x >> node.y;
      instance.nodes.push_back(node);
    }

    else if (current_section == Section::DEMAND) {
      std::istringstream ss(line);
      int id;
      int demanda;
      ss >> id >> demanda;

      for (auto& node: instance.nodes) {
        if (node.id == id) {
          node.demanda = demanda;
          break;
        }
      }
    }

    else if (current_section == Section::DEPOT) {
      std::istringstream ss(line);
      int id;
      ss >> id;

      if (!ss >> id){
        continue;
      }

      if (id == -1) {
        continue;
      }
 
      instance.depot_id = id;
    } else {
      std::istringstream ss(line);
      std::string chave, separador, valor;
      
      ss >> chave >> separador >> valor;

      if (chave == "CAPACITY") {
        std::stoi(valor, nullptr, 10); 
      }
    }
  }

  for (const auto& node : instance.nodes) {
       std::cout << "[" <<node.id << " " << node.x << " " << node.y << " " << node.demanda << "]" << std::endl;
  }

  std::cout << "Depot ID: " << instance.depot_id << std::endl;

  inFile.close();

  return 0;
}
