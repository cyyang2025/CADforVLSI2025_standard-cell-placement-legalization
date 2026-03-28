#include "Legalizer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <iomanip>

Legalizer::Legalizer(Circuit& c) : circuit(c), numBinsX(64), numBinsY(64) {
    coreMinX = coreMaxX = coreMinY = coreMaxY = 0.0;
    
    if (!circuit.rows.empty()) {
        std::sort(circuit.rows.begin(), circuit.rows.end(),
                  [](const Row& a, const Row& b) { return a.coordinate < b.coordinate; });

        coreMinY = circuit.rows.front().coordinate;
        coreMaxY = circuit.rows.back().coordinate + circuit.rows.back().height;
        coreMinX = circuit.rows.front().subrowOrigin;
        coreMaxX = coreMinX + circuit.rows.front().numSites * circuit.rows.front().siteWidth;
    }
    
    binWidth = (coreMaxX - coreMinX) / numBinsX;
    binHeight = (coreMaxY - coreMinY) / numBinsY;
    
    initialize2DBins();
}

void Legalizer::initialize2DBins() {
    bins.resize(numBinsX, std::vector<Bin2DInfo>(numBinsY));
    
    for (int x = 0; x < numBinsX; ++x) {
        for (int y = 0; y < numBinsY; ++y) {
            bins[x][y].binX = x;
            bins[x][y].binY = y;
            bins[x][y].minX = coreMinX + x * binWidth;
            bins[x][y].maxX = coreMinX + (x + 1) * binWidth;
            bins[x][y].minY = coreMinY + y * binHeight;
            bins[x][y].maxY = coreMinY + (y + 1) * binHeight;
        }
    }
}

void Legalizer::assignCellsTo2DBins() {
    for (int x = 0; x < numBinsX; ++x) {
        for (int y = 0; y < numBinsY; ++y) {
            bins[x][y].cells.clear();
        }
    }
    
    for (auto& [name, node] : circuit.nodes) {
        if (node.isFixed || node.isTerminal) continue;
        
        // Use left-corner for bin assignment (aligned with diffusion)
        double cx = node.x;
        double cy = node.y;
        
        int bx = std::clamp(int((cx - coreMinX) / binWidth), 0, numBinsX - 1);
        int by = std::clamp(int((cy - coreMinY) / binHeight), 0, numBinsY - 1);
        
        bins[bx][by].cells.push_back(&node);
    }
    
    // Sort cells by X position (left to right)
    for (int x = 0; x < numBinsX; ++x) {
        for (int y = 0; y < numBinsY; ++y) {
            std::sort(bins[x][y].cells.begin(), bins[x][y].cells.end(),
                     [](Node* a, Node* b) {
                         if (std::abs(a->x - b->x) > 1e-6) return a->x < b->x;
                         return a->y < b->y;
                     });
        }
    }
}

void Legalizer::initializeGlobalRowSegments() {
    globalRowSegments.clear();
    
    for (auto& row : circuit.rows) {
        double rowMinX = row.subrowOrigin;
        double rowMaxX = row.subrowOrigin + row.numSites * row.siteWidth;
        
        RowSegment seg;
        seg.row = &row;
        seg.startX = rowMinX;
        seg.endX = rowMaxX;
        globalRowSegments[&row].push_back(seg);
    }
    
    // Subtract space occupied by fixed cells
    for (auto const& [name, node] : circuit.nodes) {
        if (node.isFixed || node.isTerminal) {
            // Find which row this fixed cell is on
            for (auto& row : circuit.rows) {
                double rowMinY = row.coordinate;
                double rowMaxY = row.coordinate + row.height;
                
                // Check if fixed cell overlaps with this row
                if (node.y < rowMaxY && node.y + node.height > rowMinY) {
                    markSpaceUsedInRow(&row, node.x, node.width);
                }
            }
        }
    }
}

bool Legalizer::findLeftmostPositionInRow(Row* row, Node* cell, double minX, double maxX, double& outX) {
    auto it = globalRowSegments.find(row);
    if (it == globalRowSegments.end() || it->second.empty()) {
        return false;
    }
    
    double leftmostX = std::numeric_limits<double>::max();
    bool found = false;
    
    // Check all segments to find leftmost valid position
    for (const auto& seg : it->second) {
        double segMinX = std::max(seg.startX, minX);
        double segMaxX = std::min(seg.endX, maxX);
        
        if (segMaxX > segMinX) {
            double siteWidth = row->siteWidth;
            double rowOrigin = row->subrowOrigin;
            
            // Align to site grid
            double siteX = std::ceil((segMinX - rowOrigin) / siteWidth) * siteWidth + rowOrigin;
            
            // Check if this position is valid
            if (siteX < segMaxX && siteX + cell->width <= coreMaxX + 1e-6) {
                if (canPlaceCell(cell, siteX, row->coordinate)) {
                    if (siteX < leftmostX) {
                        leftmostX = siteX;
                        found = true;
                    }
                }
            }
        }
    }
    
    if (found) {
        outX = leftmostX;
        return true;
    }
    
    return false;
}

bool Legalizer::canPlaceCell(Node* cell, double x, double y) {
    const double EPSILON = 1e-6;
    
    // Check for overlaps with cells already on this row
    for (const auto& placed : globalPlacedCells) {
        if (std::abs(y - placed->y) < EPSILON) {
            double new_start = x;
            double new_end = x + cell->width;
            double old_start = placed->x;
            double old_end = placed->x + placed->width;
            
            // Check if ranges overlap
            if (!(new_end <= old_start + EPSILON || old_end <= new_start + EPSILON)) {
                return false;
            }
        }
    }
    return true;
}

void Legalizer::markSpaceUsedInRow(Row* row, double x, double width) {
    auto it = globalRowSegments.find(row);
    if (it == globalRowSegments.end()) return;
    
    std::vector<RowSegment> newSegments;
    double usedStart = x;
    double usedEnd = x + width;
    
    for (const auto& seg : it->second) {
        if (usedEnd <= seg.startX || usedStart >= seg.endX) {
            // No overlap, keep original segment
            newSegments.push_back(seg);
        } else {
            // Split segment around used space
            if (seg.startX < usedStart) {
                RowSegment leftSeg;
                leftSeg.row = row;
                leftSeg.startX = seg.startX;
                leftSeg.endX = usedStart;
                newSegments.push_back(leftSeg);
            }
            if (usedEnd < seg.endX) {
                RowSegment rightSeg;
                rightSeg.row = row;
                rightSeg.startX = usedEnd;
                rightSeg.endX = seg.endX;
                newSegments.push_back(rightSeg);
            }
        }
    }
    
    globalRowSegments[row] = newSegments;
}

void Legalizer::legalizeSingle2DBin(Bin2DInfo& bin) {
    if (bin.cells.empty()) return;
    
    std::vector<Row*> binYRows;
    for (auto& row : circuit.rows) {
        double rowMinY = row.coordinate;
        double rowMaxY = row.coordinate + row.height;
        
        if (rowMaxY > bin.minY && rowMinY < bin.maxY) {
            binYRows.push_back(&row);
        }
    }
    
    std::sort(binYRows.begin(), binYRows.end(), [](Row* a, Row* b) {
        return a->coordinate < b->coordinate;
    });
    
    int placed = 0;
    int placedInBin = 0;
    
    // PASS 1A: Y in bin, X in bin (leftmost)
    std::vector<Node*> failedPass1A;
    
    for (Node* cell : bin.cells) {
        bool placed_flag = false;
        for (Row* row : binYRows) {
            double leftmostX;
            if (findLeftmostPositionInRow(row, cell, bin.minX, bin.maxX, leftmostX)) {
                cell->x = leftmostX;
                cell->y = row->coordinate;
                markSpaceUsedInRow(row, leftmostX, cell->width);
                globalPlacedCells.push_back(cell);
                placed++;
                placedInBin++;
                placed_flag = true;
                break;
            }
        }
        if (!placed_flag) failedPass1A.push_back(cell);
    }
    
    // PASS 1B: Y in bin, X flexible ±100% (leftmost)
    double xOverHang = (bin.maxX - bin.minX) ;
    std::vector<Node*> failedPass1B;
    
    for (Node* cell : failedPass1A) {
        bool placed_flag = false;
        for (Row* row : binYRows) {
            double leftmostX;
            if (findLeftmostPositionInRow(row, cell, bin.minX - xOverHang, bin.maxX + xOverHang, leftmostX)) {
                cell->x = leftmostX;
                cell->y = row->coordinate;
                markSpaceUsedInRow(row, leftmostX, cell->width);
                globalPlacedCells.push_back(cell);
                placed++;
                placedInBin++;
                placed_flag = true;
                break;
            }
        }
        if (!placed_flag) failedPass1B.push_back(cell);
    }
    
    // PASS 2: Any nearest row, X flexible ±100% (leftmost)
    std::vector<Node*> failedPass2;
    
    for (Node* cell : failedPass1B) {
        double cellY = cell->y;
        bool placed_flag = false;
        
        std::vector<std::pair<double, Row*>> rowsByDist;
        for (auto& row : circuit.rows) {
            double dist = std::abs(cellY - row.coordinate);
            rowsByDist.push_back({dist, &row});
        }
        std::sort(rowsByDist.begin(), rowsByDist.end(), 
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        for (const auto& [dist, row] : rowsByDist) {
            double leftmostX;
            if (findLeftmostPositionInRow(row, cell, bin.minX - xOverHang, bin.maxX + xOverHang, leftmostX)) {
                cell->x = leftmostX;
                cell->y = row->coordinate;
                markSpaceUsedInRow(row, leftmostX, cell->width);
                globalPlacedCells.push_back(cell);
                placed++;
                placedInBin++;
                placed_flag = true;
                break;
            }
        }
        if (!placed_flag) failedPass2.push_back(cell);
    }
    
    // PASS 3: Any row, any X (leftmost)
    int pass3 = 0;
    
    for (Node* cell : failedPass2) {
        double cellY = cell->y;
        
        std::vector<std::pair<double, Row*>> rowsByDist;
        for (auto& row : circuit.rows) {
            double dist = std::abs(cellY - row.coordinate);
            rowsByDist.push_back({dist, &row});
        }
        std::sort(rowsByDist.begin(), rowsByDist.end(), 
                 [](const auto& a, const auto& b) { return a.first < b.first; });
        
        for (const auto& [dist, row] : rowsByDist) {
            double leftmostX;
            if (findLeftmostPositionInRow(row, cell, coreMinX, coreMaxX, leftmostX)) {
                cell->x = leftmostX;
                cell->y = row->coordinate;
                markSpaceUsedInRow(row, leftmostX, cell->width);
                globalPlacedCells.push_back(cell);
                placed++;
                pass3++;
                break;
            }
        }
    }
}

void Legalizer::legalizeBinByBin() {
    initializeGlobalRowSegments();
    globalPlacedCells.clear();
    
    for (int x = 0; x < numBinsX; ++x) {
        for (int y = 0; y < numBinsY; ++y) {
            if (!bins[x][y].cells.empty()) {
                legalizeSingle2DBin(bins[x][y]);
            }
        }
    }
}

void Legalizer::reportResults() {
    int totalMovable = 0;
    int legalPlaced = 0;
    
    for (auto const& [name, node] : circuit.nodes) {
        if (node.isFixed || node.isTerminal) continue;
        totalMovable++;
        
        bool onValidRow = false;
        for (auto& row : circuit.rows) {
            if (std::abs(node.y - row.coordinate) < 1e-3) {
                onValidRow = true;
                break;
            }
        }
        if (onValidRow) legalPlaced++;
    }
}

void Legalizer::run() {
    assignCellsTo2DBins();
    legalizeBinByBin();
    reportResults();
}
