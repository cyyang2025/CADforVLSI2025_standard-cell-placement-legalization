#include "Writer.h"
#include <fstream>
#include <iomanip>
#include <iostream>

Writer::Writer(const Circuit& c, 
               const std::string& p,
               const std::string& inputNodes,
               const std::string& inputNets,
               const std::string& inputScl,
               const std::string& inputWts) 
    : circuit(c), 
      prefix(p), 
      inputNodesPath(inputNodes),
      inputNetsPath(inputNets),
      inputSclPath(inputScl),
      inputWtsPath(inputWts) {
    
    // Initialize cellOrder with all cells from circuit
    for (const auto& [name, node] : circuit.nodes) {
        cellOrder.push_back(name);
    }
}

void Writer::writeAll() {
    writeAux();
    copyNodes();    // Copy instead of write
    writePl();      // This one changes (legalized positions)
    copyNets();     // Copy instead of write
    copyScl();      // Copy instead of write
    copyWts();      // Copy instead of write
}

void Writer::writeAux() {
    std::ofstream fout(prefix + ".aux");
    fout << "RowBasedPlacement : " 
         << prefix << ".nodes " 
         << prefix << ".nets "
         << prefix << ".wts "
         << prefix << ".pl "
         << prefix << ".scl " << std::endl;
}

void Writer::setPl(const std::vector<std::string>& order) {
    cellOrder = order;
}

void Writer::writePl() {
    std::ofstream fout(prefix + ".pl");
    fout << "UCLA pl 1.0" << std::endl << std::endl;
    
    // Write cells in original order
    for (const auto& name : cellOrder) {
        if (circuit.nodes.find(name) == circuit.nodes.end()) continue;
        
        const auto& node = circuit.nodes.at(name);
        fout << name << "\t" << std::fixed << std::setprecision(1) 
             << node.x << "\t" << node.y << "\t: " << node.orient;
        if (node.isFixed) {
            fout << "\t/FIXED";
        }
        fout << std::endl;
    }
}

// Helper function to copy files byte-by-byte
bool Writer::copyFile(const std::string& source, const std::string& dest) {
    std::ifstream src(source, std::ios::binary);
    std::ofstream dst(dest, std::ios::binary);
    
    if (!src.is_open()) {
        std::cerr << "Warning: Could not open source file: " << source << std::endl;
        return false;
    }
    
    // Copy the entire file
    dst << src.rdbuf();
    
    return true;
}

void Writer::copyNodes() {
    if (!copyFile(inputNodesPath, prefix + ".nodes")) {
        std::cerr << "Failed to copy .nodes file" << std::endl;
    }
}

void Writer::copyNets() {
    if (!copyFile(inputNetsPath, prefix + ".nets")) {
        std::cerr << "Failed to copy .nets file" << std::endl;
    }
}

void Writer::copyScl() {
    if (!copyFile(inputSclPath, prefix + ".scl")) {
        std::cerr << "Failed to copy .scl file" << std::endl;
    }
}

void Writer::copyWts() {
    if (!copyFile(inputWtsPath, prefix + ".wts")) {
        std::cerr << "Failed to copy .wts file" << std::endl;
    }
}
