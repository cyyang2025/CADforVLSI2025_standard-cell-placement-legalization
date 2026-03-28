#include "Parser.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <algorithm>

// ---------------- Helper Functions ----------------

bool Parser::endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

double Parser::parseDouble(const std::string& line) {
    return std::stod(line.substr(line.find(':') + 1));
}

std::string Parser::parseString(const std::string& line) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) return "";
    
    std::string value = line.substr(colonPos + 1);
    // Trim leading and trailing whitespace
    size_t start = value.find_first_not_of(" \t");
    size_t end = value.find_last_not_of(" \t\r\n");
    
    if (start == std::string::npos) return "";
    return value.substr(start, end - start + 1);
}

// ---------------- Main Parse Entry ----------------

void Parser::parseAll(const std::string& aux_filename, Circuit& circuit) {
    // Extract the base path from the .aux filename
    size_t last_slash_idx = aux_filename.find_last_of("/");
    if (std::string::npos != last_slash_idx) {
        basePath = aux_filename.substr(0, last_slash_idx + 1);
    }

    parseAux(aux_filename, circuit);

    for (const auto& f : files) {
        std::string full_path = basePath + f;

        if (endsWith(f, ".nodes")) parseNodes(full_path, circuit);
        else if (endsWith(f, ".pl")) parsePl(full_path, circuit);
        else if (endsWith(f, ".nets")) parseNets(full_path, circuit);
        else if (endsWith(f, ".scl")) parseScl(full_path, circuit);
        else if (endsWith(f, ".wts")) parseWts(full_path, circuit);
    }
}

// ---------------- Sub-Parser Implementations ----------------

void Parser::parseAux(const std::string& filename, Circuit&) {
    std::ifstream fin(filename);
    if (!fin.is_open()) throw std::runtime_error("Cannot open AUX: " + filename);

    std::string line; std::getline(fin, line);
    if (line.find("RowBasedPlacement") == std::string::npos)
        throw std::runtime_error("Invalid AUX header");

    std::stringstream ss(line);
    std::string token;
    ss >> token >> token;  // Skip "RowBasedPlacement" and ":"
    while (ss >> token) files.push_back(token);
}

void Parser::parseNodes(const std::string& filename, Circuit& circuit) {
    std::ifstream fin(filename);
    if (!fin.is_open()) throw std::runtime_error("Cannot open NODES: " + filename);

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#' ||
            line.find("UCLA") != std::string::npos ||
            line.find("Num") != std::string::npos)
            continue;

        std::stringstream ss(line);
        Node node;
        ss >> node.name >> node.width >> node.height;
        std::string maybe_terminal;
        if (ss >> maybe_terminal)
            if (maybe_terminal == "terminal") node.isTerminal = true;
        circuit.nodes[node.name] = node;
    }
}

void Parser::parsePl(const std::string& filename, Circuit& circuit) {
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    std::getline(fin, line);  // Skip header "UCLA pl 1.0"

    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string name, orient;
        double x, y;
        char colon;

        iss >> name >> x >> y >> colon >> orient;

        if (circuit.nodes.find(name) != circuit.nodes.end()) {
            circuit.nodes[name].x = x;
            circuit.nodes[name].y = y;
            circuit.nodes[name].orient = orient;

            std::string fixed;
            if (iss >> fixed && fixed == "/FIXED") {
                circuit.nodes[name].isFixed = true;
            }

            cellOrder.push_back(name);  //Store order
        }
    }

    fin.close();
}


void Parser::parseNets(const std::string& filename, Circuit& circuit) {
    std::ifstream fin(filename);
    if (!fin.is_open()) throw std::runtime_error("Cannot open NETS: " + filename);

    std::string line;
    Net current;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#' ||
            line.find("UCLA") != std::string::npos)
            continue;

        if (line.find("NetDegree") != std::string::npos) {
            if (current.degree > 0) circuit.nets.push_back(current);
            std::stringstream ss(line);
            std::string tmp, colon;
            ss >> tmp >> colon >> current.degree;
            ss >> current.name;
            current.pins.clear();
        } else {
            std::stringstream ss(line);
            Pin p; char colon;
            ss >> p.node_name >> p.direction >> colon >> p.x_offset >> p.y_offset;
            current.pins.push_back(p);
        }
    }
    if (current.degree > 0) circuit.nets.push_back(current);
}

void Parser::parseScl(const std::string& filename, Circuit& circuit) {
    std::ifstream fin(filename);
    if (!fin.is_open()) throw std::runtime_error("Cannot open SCL: " + filename);

    std::string line;
    Row row;
    while (std::getline(fin, line)) {
        if (line.find("CoreRow") != std::string::npos) {
            row = Row();  // Reset to default values
        }
        else if (line.find("Coordinate") != std::string::npos)
            row.coordinate = parseDouble(line);
        else if (line.find("Height") != std::string::npos)
            row.height = parseDouble(line);
        else if (line.find("Sitewidth") != std::string::npos)
            row.siteWidth = parseDouble(line);
        else if (line.find("Sitespacing") != std::string::npos)
            row.siteSpacing = parseDouble(line);
        else if (line.find("Siteorient") != std::string::npos)
            row.siteOrient = parseString(line);
        else if (line.find("Sitesymmetry") != std::string::npos)
            row.siteSymmetry = parseString(line);
        else if (line.find("SubrowOrigin") != std::string::npos) {
            size_t posA = line.find(":");
            size_t posB = line.find("NumSites");
            row.subrowOrigin = std::stod(line.substr(posA + 1, posB - posA));
            row.numSites = std::stoi(line.substr(line.find_last_of(":") + 1));
        } 
        else if (line.find("End") != std::string::npos)
            circuit.rows.push_back(row);
    }
    if (!circuit.rows.empty()) {
        std::sort(circuit.rows.begin(), circuit.rows.end(),
              [](const Row& a, const Row& b){ return a.coordinate < b.coordinate; });
}

}

void Parser::parseWts(const std::string& filename, Circuit& circuit) {
    std::ifstream fin(filename);
    if (!fin.is_open()) return; // optional file

    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty() || line[0] == '#' ||
            line.find("UCLA") != std::string::npos)
            continue;

        std::stringstream ss(line);
        std::string name;
        double weight;
        ss >> name >> weight;
        if (circuit.nodes.count(name))
            circuit.nodes[name].weight = weight;
        else {
            for (auto& net : circuit.nets)
                if (net.name == name) net.degree = weight;
        }
    }
}
