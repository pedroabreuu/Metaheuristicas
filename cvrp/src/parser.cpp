#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

enum class Section { NONE, NODE_COORD, DEMAND, DEPOT };

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

VRPInstance parseVRP(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + filepath);
    }

    VRPInstance instance;
    Section current_section = Section::NONE;
    std::string line;

    while (getline(inFile, line)) {
        if (trim(line) == "NODE_COORD_SECTION") { current_section = Section::NODE_COORD; continue; }
        else if (trim(line) == "DEMAND_SECTION") { current_section = Section::DEMAND;     continue; }
        else if (trim(line) == "DEPOT_SECTION")  { current_section = Section::DEPOT;      continue; }

        if (current_section == Section::NODE_COORD) {
            std::istringstream ss(line);
            Node node;
            ss >> node.id >> node.x >> node.y;
            instance.nodes.push_back(node);
        }
        else if (current_section == Section::DEMAND) {
            std::istringstream ss(line);
            int id, demanda;
            ss >> id >> demanda;
            for (auto& node : instance.nodes) {
                if (node.id == id) { node.demanda = demanda; break; }
            }
        }
        else if (current_section == Section::DEPOT) {
            std::istringstream ss(line);
            int id;
            if (!(ss >> id) || id == -1) continue;
            instance.depot_id = id;
        }
        else {
            std::istringstream ss(line);
            std::string chave, separador, valor;
            ss >> chave >> separador >> valor;

            if (chave == "CAPACITY") {
                instance.capacity = std::stoi(valor);
            }
            else if (chave == "COMMENT") {
                size_t pos = line.find("No of trucks");
                if (pos != std::string::npos) {
                    pos = line.find(':', pos);
                    instance.num_trucks = std::stoi(line.substr(pos + 1));
                }
                size_t posOpt = line.find("Optimal value");
                if (posOpt != std::string::npos) {
                    posOpt = line.find(':', posOpt);
                    instance.optimal_value = std::stoi(line.substr(posOpt + 1));
                }
            }
        }
    }

    inFile.close();
    instance.buildIndex();
    return instance;
}
