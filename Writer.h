#ifndef WRITER_H
#define WRITER_H

#include "Parser.h"
#include <string>
#include <vector>

class Writer {
public:
    // Modified constructor: takes input file paths
    Writer(const Circuit& circuit, 
           const std::string& outputPrefix,
           const std::string& inputNodesFile,
           const std::string& inputNetsFile,
           const std::string& inputSclFile,
           const std::string& inputWtsFile);
    
    void writeAll();
    void setPl(const std::vector<std::string>& order);

private:
    const Circuit& circuit;
    std::string prefix;
    std::string inputNodesPath;
    std::string inputNetsPath;
    std::string inputSclPath;
    std::string inputWtsPath;
    std::vector<std::string> cellOrder;

    void writeAux();
    void writePl();
    
    // Copy functions instead of write functions
    void copyNodes();
    void copyNets();
    void copyScl();
    void copyWts();
    
    // Helper function to copy files
    bool copyFile(const std::string& source, const std::string& dest);
};

#endif // WRITER_H
