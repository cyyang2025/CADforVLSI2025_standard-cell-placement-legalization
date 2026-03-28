#ifndef LEGALIZER_H
#define LEGALIZER_H

#include "Parser.h"
#include <vector>
#include <map>

class Legalizer {
public:
    Legalizer(Circuit& c);
    void run();

private:
    Circuit& circuit;
    
    double coreMinX, coreMaxX, coreMinY, coreMaxY;
    double binWidth, binHeight;
    int numBinsX, numBinsY;
    
    struct RowSegment {
        Row* row;
        double startX;
        double endX;
    };
    
    struct Bin2DInfo {
        int binX, binY;
        double minX, maxX, minY, maxY;
        std::vector<Node*> cells;
    };
    
    std::vector<std::vector<Bin2DInfo>> bins;
    std::map<Row*, std::vector<RowSegment>> globalRowSegments;
    std::vector<Node*> globalPlacedCells;
    
    void initialize2DBins();
    void assignCellsTo2DBins();
    void initializeGlobalRowSegments();
    void legalizeBinByBin();
    void legalizeSingle2DBin(Bin2DInfo& bin);
    bool findLeftmostPositionInRow(Row* row, Node* cell, double minX, double maxX, double& outX);
    bool canPlaceCell(Node* cell, double x, double y);
    void markSpaceUsedInRow(Row* row, double x, double width);
    void reportResults();
};

#endif
