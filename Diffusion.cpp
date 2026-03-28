#include "Diffusion.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <numeric>

Diffusion::Diffusion(Circuit& c, const DiffusionParams& p)
    : circuit(c), params(p), totalMovableArea(0.0), totalUsableRowArea(0.0) {
    
    coreMinX = coreMaxX = coreMinY = coreMaxY = 0.0;
    
    if (!circuit.rows.empty()) {
        std::sort(circuit.rows.begin(), circuit.rows.end(),
                  [](const Row& a, const Row& b) { return a.coordinate < b.coordinate; });
        
        coreMinY = circuit.rows.front().coordinate;
        coreMaxY = circuit.rows.back().coordinate + circuit.rows.back().height;
        coreMinX = circuit.rows.front().subrowOrigin;
        coreMaxX = coreMinX + circuit.rows.front().numSites * circuit.rows.front().siteWidth;
    }
    
    initialize();
}

void Diffusion::initialize() {
    chipWidth = coreMaxX - coreMinX;
    chipHeight = coreMaxY - coreMinY;
    binWidth = chipWidth / params.numBinsX;
    binHeight = chipHeight / params.numBinsY;
    
    // Calculate total usable row area
    totalUsableRowArea = 0.0;
    for (auto& row : circuit.rows) {
        totalUsableRowArea += row.height * row.numSites * row.siteWidth;
    }
    
    // Subtract fixed cell area
    double fixedCellArea = 0.0;
    for (auto const& [name, n] : circuit.nodes) {
        if (n.isFixed || n.isTerminal) {
            double cellLeft = n.x;
            double cellRight = n.x + n.width;
            double cellBottom = n.y;
            double cellTop = n.y + n.height;
            
            for (auto& row : circuit.rows) {
                double rowMinY = row.coordinate;
                double rowMaxY = row.coordinate + row.height;
                
                if (cellTop > rowMinY && cellBottom < rowMaxY) {
                    double overlapBottom = std::max(cellBottom, rowMinY);
                    double overlapTop = std::min(cellTop, rowMaxY);
                    double overlapHeight = overlapTop - overlapBottom;
                    
                    double rowMinX = row.subrowOrigin;
                    double rowMaxX = row.subrowOrigin + row.numSites * row.siteWidth;
                    
                    if (cellRight > rowMinX && cellLeft < rowMaxX) {
                        double overlapLeft = std::max(cellLeft, rowMinX);
                        double overlapRight = std::min(cellRight, rowMaxX);
                        double overlapWidth = overlapRight - overlapLeft;
                        fixedCellArea += overlapWidth * overlapHeight;
                    }
                }
            }
        }
    }
    
    totalUsableRowArea -= fixedCellArea;
    
    totalMovableArea = 0.0;
    for (auto const& [name, n] : circuit.nodes) {
        if (!n.isFixed && !n.isTerminal) {
            totalMovableArea += n.width * n.height;
        }
    }
    
    bins.resize(params.numBinsX, std::vector<Bin2D>(params.numBinsY));
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            bins[x][y].binX = x;
            bins[x][y].binY = y;
            bins[x][y].minX = coreMinX + x * binWidth;
            bins[x][y].maxX = coreMinX + (x + 1) * binWidth;
            bins[x][y].minY = coreMinY + y * binHeight;
            bins[x][y].maxY = coreMinY + (y + 1) * binHeight;
            bins[x][y].isFixed = false;
        }
    }
    
    computeBinCapacities();
    
    // Initialize density grid
    densityGrid.density.resize(params.numBinsX, std::vector<double>(params.numBinsY, 0.0));
    densityGrid.velocityH.resize(params.numBinsX, std::vector<double>(params.numBinsY, 0.0));
    densityGrid.velocityV.resize(params.numBinsX, std::vector<double>(params.numBinsY, 0.0));
}

void Diffusion::computeBinCapacities() {
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            double binCapacity = 0.0;
            
            for (auto& row : circuit.rows) {
                double rowMinY = row.coordinate;
                double rowMaxY = row.coordinate + row.height;
                double binMinY = bins[x][y].minY;
                double binMaxY = bins[x][y].maxY;
                
                if (rowMaxY > binMinY && rowMinY < binMaxY) {
                    double overlapMinY = std::max(rowMinY, binMinY);
                    double overlapMaxY = std::min(rowMaxY, binMaxY);
                    double overlapHeight = overlapMaxY - overlapMinY;
                    
                    double rowMinX = row.subrowOrigin;
                    double rowMaxX = row.subrowOrigin + row.numSites * row.siteWidth;
                    double binMinX = bins[x][y].minX;
                    double binMaxX = bins[x][y].maxX;

                    double overlapMinX = std::max(rowMinX, binMinX);
                    double overlapMaxX = std::min(rowMaxX, binMaxX);
                    double overlapWidth = std::max(0.0, overlapMaxX - overlapMinX);

                    binCapacity += overlapWidth * overlapHeight;
                }
            }
            
            // Subtract fixed cell area
            double fixedAreaInBin = 0.0;
            for (auto const& [name, n] : circuit.nodes) {
                if (n.isFixed || n.isTerminal) {
                    double cellLeft = n.x;
                    double cellRight = n.x + n.width;
                    double cellBottom = n.y;
                    double cellTop = n.y + n.height;
                    
                    double binLeft = bins[x][y].minX;
                    double binRight = bins[x][y].maxX;
                    double binBottom = bins[x][y].minY;
                    double binTop = bins[x][y].maxY;
                    
                    if (cellRight > binLeft && cellLeft < binRight &&
                        cellTop > binBottom && cellBottom < binTop) {
                        double overlapLeft = std::max(cellLeft, binLeft);
                        double overlapRight = std::min(cellRight, binRight);
                        double overlapBottom = std::max(cellBottom, binBottom);
                        double overlapTop = std::min(cellTop, binTop);
                        
                        double overlapWidth = overlapRight - overlapLeft;
                        double overlapHeight = overlapTop - overlapBottom;
                        fixedAreaInBin += overlapWidth * overlapHeight;
                    }
                }
            }
            
            bins[x][y].capacity = std::max(0.0, binCapacity - fixedAreaInBin);
        }
    }
}

void Diffusion::assignCellsToBins() {
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            bins[x][y].cells.clear();
        }
    }
    
    for (auto& [name, node] : circuit.nodes) {
        if (node.isFixed || node.isTerminal) continue;
        
        double cellX = node.x;
        double cellY = node.y;
        
        int bx = std::clamp(int((cellX - coreMinX) / binWidth), 0, params.numBinsX - 1);
        int by = std::clamp(int((cellY - coreMinY) / binHeight), 0, params.numBinsY - 1);
        
        bins[bx][by].cells.push_back(&node);
    }
}

void Diffusion::computeInitialDensity() {
    // Initialize density for all bins
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            densityGrid.density[x][y] = 0.0;
        }
    }
    
    // For each ALL cell, add its intersection area to ALL bins it overlaps
    for (auto& [name, node] : circuit.nodes) {
        
        double cellLeft = node.x;
        double cellRight = node.x + node.width;
        double cellBottom = node.y;
        double cellTop = node.y + node.height;
        
        // Find which bins this cell overlaps
        int binMinX = std::clamp(int((cellLeft - coreMinX) / binWidth), 0, params.numBinsX - 1);
        int binMaxX = std::clamp(int((cellRight - coreMinX) / binWidth), 0, params.numBinsX - 1);
        int binMinY = std::clamp(int((cellBottom - coreMinY) / binHeight), 0, params.numBinsY - 1);
        int binMaxY = std::clamp(int((cellTop - coreMinY) / binHeight), 0, params.numBinsY - 1);
        
        // Add intersection area to each overlapping bin
        for (int bx = binMinX; bx <= binMaxX; ++bx) {
            for (int by = binMinY; by <= binMaxY; ++by) {
                double binLeft = bins[bx][by].minX;
                double binRight = bins[bx][by].maxX;
                double binBottom = bins[bx][by].minY;
                double binTop = bins[bx][by].maxY;
                
                // Calculate intersection
                double overlapLeft = std::max(cellLeft, binLeft);
                double overlapRight = std::min(cellRight, binRight);
                double overlapBottom = std::max(cellBottom, binBottom);
                double overlapTop = std::min(cellTop, binTop);
                
                double overlapWidth = std::max(0.0, overlapRight - overlapLeft);
                double overlapHeight = std::max(0.0, overlapTop - overlapBottom);
                double intersectionArea = overlapWidth * overlapHeight;
                
                // Add to bin density
                if (bins[bx][by].capacity > 0) {
                    densityGrid.density[bx][by] += intersectionArea / bins[bx][by].capacity;
                }
            }
        }
    }
}


// Equation (8): Manipulate density map to avoid over-spreading
void Diffusion::manipulateDensityMap() {
    double totalDensity = 0.0;
    double overflowArea = 0.0;
    double underflowArea = 0.0;
    
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            totalDensity += densityGrid.density[x][y];
            
            if (densityGrid.density[x][y] > params.targetDensity) {
                overflowArea += (densityGrid.density[x][y] - params.targetDensity) * bins[x][y].capacity;
            } else {
                underflowArea += (params.targetDensity - densityGrid.density[x][y]) * bins[x][y].capacity;
            }
        }
    }
    
    double avgDensity = totalDensity / (params.numBinsX * params.numBinsY);
    
    if (avgDensity >= params.targetDensity) return;
    
    // Adjust under-full bins
    if (underflowArea > 0) {
        for (int x = 0; x < params.numBinsX; ++x) {
            for (int y = 0; y < params.numBinsY; ++y) {
                if (densityGrid.density[x][y] < params.targetDensity) {
                    double adjustedDensity = params.targetDensity - 
                        (params.targetDensity - densityGrid.density[x][y]) * 
                        (overflowArea / underflowArea);
                    densityGrid.density[x][y] = adjustedDensity;
                }
            }
        }
    }
}

// Equation (4): Discretize diffusion equation using FTCS scheme
void Diffusion::diffusionStep(DensityGrid& grid) {
    std::vector<std::vector<double>> newDensity = grid.density;
    
    for (int x = 1; x < params.numBinsX - 1; ++x) {
        for (int y = 1; y < params.numBinsY - 1; ++y) {
            if (bins[x][y].isFixed) continue;
            
            // FTCS scheme: d(n+1) = d(n) + (Δt/2)[d_x + Δt/2][d_y + Δt/2]
            double dH = params.deltaT / 2.0 * (grid.density[x+1][y] + grid.density[x-1][y] - 2*grid.density[x][y]);
            double dV = params.deltaT / 2.0 * (grid.density[x][y+1] + grid.density[x][y-1] - 2*grid.density[x][y]);
            
            newDensity[x][y] = grid.density[x][y] + dH + dV;
        }
    }
    
    // Boundary conditions: ∇d = 0 at boundaries
    for (int x = 0; x < params.numBinsX; ++x) {
        newDensity[x][0] = newDensity[x][1];
        newDensity[x][params.numBinsY-1] = newDensity[x][params.numBinsY-2];
    }
    for (int y = 0; y < params.numBinsY; ++y) {
        newDensity[0][y] = newDensity[1][y];
        newDensity[params.numBinsX-1][y] = newDensity[params.numBinsX-2][y];
    }
    
    grid.density = newDensity;
}

// Equation (5): Compute velocities based on density gradient
void Diffusion::computeVelocities(DensityGrid& grid) {
    for (int x = 1; x < params.numBinsX - 1; ++x) {
        for (int y = 1; y < params.numBinsY - 1; ++y) {
            if (grid.density[x][y] < 1e-12) {
                grid.velocityH[x][y] = 0.0;
                grid.velocityV[x][y] = 0.0;
            } else {
                // vH = -(d_right - d_left) / (2*d)
                grid.velocityH[x][y] = -(grid.density[x+1][y] - grid.density[x-1][y]) / (2.0 * grid.density[x][y]);
                // vV = -(d_top - d_bottom) / (2*d)
                grid.velocityV[x][y] = -(grid.density[x][y+1] - grid.density[x][y-1]) / (2.0 * grid.density[x][y]);
            }
        }
    }
}

// Equation (6): Interpolate velocity at arbitrary position
double Diffusion::interpolateVelocity(double x, double y, bool isHorizontal, const DensityGrid& grid) {
    // First, clamp input to valid range
    double clampedX = std::max(0.0, std::min((double)(params.numBinsX - 1), x));
    double clampedY = std::max(0.0, std::min((double)(params.numBinsY - 1), y));
    
    int p = (int)std::floor(clampedX);
    int q = (int)std::floor(clampedY);
    
    // Ensure we stay in valid range for accessing 4 neighbors
    p = std::max(0, std::min(p, params.numBinsX - 2));
    q = std::max(0, std::min(q, params.numBinsY - 2));
    
    double alpha = clampedX - p;
    double beta = clampedY - q;
    
    double v_pq, v_p1q, v_pq1, v_p1q1;
    
    if (isHorizontal) {
        v_pq = grid.velocityH[p][q];
        v_p1q = grid.velocityH[p+1][q];
        v_pq1 = grid.velocityH[p][q+1];
        v_p1q1 = grid.velocityH[p+1][q+1];
    } else {
        v_pq = grid.velocityV[p][q];
        v_p1q = grid.velocityV[p+1][q];
        v_pq1 = grid.velocityV[p][q+1];
        v_p1q1 = grid.velocityV[p+1][q+1];
    }
    
    // Bilinear interpolation
    double v = v_pq + alpha * (v_p1q - v_pq) + beta * (v_pq1 - v_pq) + 
               alpha * beta * (v_pq + v_p1q1 - v_p1q - v_pq1);
    
    return v;
}

// Equation (7): Update cell locations (using LEFT-CORNER method)
void Diffusion::updateCellLocations(const DensityGrid& grid, double deltaT) {
    // Clear bins
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            bins[x][y].cells.clear();
        }
    }
    
    // Move cells and reassign to bins
    for (auto& [name, node] : circuit.nodes) {
        if (node.isFixed || node.isTerminal) continue;
        
        double binX = (node.x - coreMinX) / binWidth;
        double binY = (node.y - coreMinY) / binHeight;
        
        double vH = interpolateVelocity(binX, binY, true, grid);
        double vV = interpolateVelocity(binX, binY, false, grid);
        
        node.x = (binX + vH * deltaT) * binWidth + coreMinX;
        node.y = (binY + vV * deltaT) * binHeight + coreMinY;
        
        // Clamp to boundaries
        node.x = std::max(coreMinX, std::min(coreMaxX - node.width, node.x));
        node.y = std::max(coreMinY, std::min(coreMaxY - node.height, node.y));
        
        // Reassign to new bin
        int newBx = std::clamp((int)((node.x - coreMinX) / binWidth), 
                               0, params.numBinsX - 1);
        int newBy = std::clamp((int)((node.y - coreMinY) / binHeight), 
                               0, params.numBinsY - 1);
        
        bins[newBx][newBy].cells.push_back(&node);
    }
}

double Diffusion::calculateDensityOverflow() {
    double totalOverflow = 0.0;
    
    for (int x = 0; x < params.numBinsX; ++x) {
        for (int y = 0; y < params.numBinsY; ++y) {
            if (densityGrid.density[x][y] > params.targetDensity) {
                totalOverflow += densityGrid.density[x][y] - params.targetDensity;
            }
        }
    }
    
    return totalOverflow;
}

void Diffusion::globalDiffusion() {
    
    computeInitialDensity();
    manipulateDensityMap();
    
    int iteration = 0;
    
    while (iteration < params.maxIterations) {
        computeVelocities(densityGrid);
        updateCellLocations(densityGrid, params.deltaT);
        diffusionStep(densityGrid);
        
        double maxDensity = 0.0;
        for (int x = 0; x < params.numBinsX; ++x) {
            for (int y = 0; y < params.numBinsY; ++y) {
                maxDensity = std::max(maxDensity, densityGrid.density[x][y]);
            }
        }
        
        iteration++;
    }
}

void Diffusion::run() {
    globalDiffusion();
}