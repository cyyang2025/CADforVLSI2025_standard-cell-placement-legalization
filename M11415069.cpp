#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include "Parser.h"
#include "Diffusion.h"
#include "Legalizer.h"
#include "Writer.h"

struct Point {
    double x, y;
};

std::string getDirectory(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash == std::string::npos) {
        return "./";
    }
    return filepath.substr(0, lastSlash + 1);
}

std::string getBaseName(const std::string& filepath) {
    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of(".");
    
    size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
    size_t end = (lastDot == std::string::npos) ? filepath.length() : lastDot;
    
    return filepath.substr(start, end - start);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.aux> <output_prefix>" << std::endl;
        return 1;
    }

    std::string inputAux = argv[1];
    std::string outputPrefix = argv[2];

    std::string inputDir = getDirectory(inputAux);
    std::string baseName = getBaseName(inputAux);
    
    std::string inputNodesFile = inputDir + baseName + ".nodes";
    std::string inputNetsFile = inputDir + baseName + ".nets";
    std::string inputSclFile = inputDir + baseName + ".scl";
    std::string inputWtsFile = inputDir + baseName + ".wts";

    Circuit circuit;
    Parser parser;
    try {
        parser.parseAll(inputAux, circuit);
    } catch (const std::runtime_error& e) {
        return 1;
    }

    // Get cell order from parser
    std::vector<std::string> cellOrder = parser.getCellOrder();

    // Store initial positions before spreading and legalization
    std::unordered_map<std::string, Point> initialPositions;
    for (const auto& [name, node] : circuit.nodes) {
        if (!node.isTerminal && !node.isFixed) {
            initialPositions[name] = {node.x, node.y};
        }
    }

    DiffusionParams diffusionParams;
    
    Diffusion diffusion(circuit, diffusionParams);
    diffusion.run();

    Legalizer legalizer(circuit);
    legalizer.run();

    double totalDisplacement = 0.0;
    double maxDisplacement = 0.0;

    for (const auto& [name, initialPos] : initialPositions) {
        const auto& finalNode = circuit.nodes.at(name);
        double dx = std::abs(finalNode.x - initialPos.x);
        double dy = std::abs(finalNode.y - initialPos.y);
        double displacement = dx + dy;  // Manhattan distance
        
        totalDisplacement += displacement;
        
        if (displacement > maxDisplacement) {
            maxDisplacement = displacement;
        }
    }

    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Total displacement: " << totalDisplacement << std::endl;
    std::cout << "Maximum displacement: " << maxDisplacement << std::endl;

    Writer writer(circuit, outputPrefix, 
                  inputNodesFile, inputNetsFile, inputSclFile, inputWtsFile);
    writer.setPl(cellOrder);
    writer.writeAll();

    return 0;
}