#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <unordered_map>

// ---------------- Data Structures ----------------

struct Pin {
    std::string node_name;
    char direction = 'I';        // 'I' or 'O'
    double x_offset = 0.0;
    double y_offset = 0.0;
};

struct Net {
    std::string name;
    int degree = 0;
    std::vector<Pin> pins;
};

struct Node {
    std::string name;
    double width = 0.0;
    double height = 0.0;
    double x = 0.0;
    double y = 0.0;
    std::string orient = "N";
    double weight = 1.0;
    bool isTerminal = false;
    bool isFixed = false;
};

struct Row {
    double coordinate = 0.0;
    double height = 0.0;
    double siteWidth = 0.0;
    double siteSpacing = 0.0;
    std::string siteOrient = "";
    std::string siteSymmetry = "";
    double subrowOrigin = 0.0;
    int numSites = 0;
};

struct Circuit {
    std::unordered_map<std::string, Node> nodes;
    std::vector<Net> nets;
    std::vector<Row> rows;
};

// ---------------- Parser Declaration ----------------

class Parser {
public:
    void parseAll(const std::string& aux_filename, Circuit& circuit);
    
    std::vector<std::string> getCellOrder() const {
        return cellOrder;
    }

private:
    std::vector<std::string> files;
    std::string basePath;

    // Sub-parsers for each file type
    void parseAux(const std::string& filename, Circuit& circuit);
    void parseNodes(const std::string& filename, Circuit& circuit);
    void parsePl(const std::string& filename, Circuit& circuit);
    void parseNets(const std::string& filename, Circuit& circuit);
    void parseScl(const std::string& filename, Circuit& circuit);
    void parseWts(const std::string& filename, Circuit& circuit);

    // Helpers
    bool endsWith(const std::string& s, const std::string& suffix);
    static double parseDouble(const std::string& line);
    static std::string parseString(const std::string& line);
    std::vector<std::string> cellOrder;
};

#endif // PARSER_H
