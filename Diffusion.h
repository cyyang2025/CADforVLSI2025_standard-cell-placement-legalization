#ifndef DIFFUSION_H
#define DIFFUSION_H

#include "Parser.h"
#include <vector>
#include <map>


struct DiffusionParams {
    int numBinsX = 128;
    int numBinsY = 128;
    int maxIterations = 200;
    double deltaT = 0.05;
    double targetDensity = 1.0;
};


class Diffusion {
public:
    Diffusion(Circuit& c, const DiffusionParams& p);
    void run();

private:
    Circuit& circuit;
    const DiffusionParams& params;

    double coreMinX, coreMaxX, coreMinY, coreMaxY;
    double chipWidth, chipHeight;
    double binWidth, binHeight;
    double totalMovableArea, totalUsableRowArea;


    struct Bin2D {
        int binX, binY;
        double minX, maxX, minY, maxY;
        std::vector<Node*> cells;
        double density;  // Current bin density
        double capacity;  // Bin capacity
        bool isFixed;  // Whether this bin is fixed (no diffusion)
    };
    
    struct DensityGrid {
        std::vector<std::vector<double>> density;  // Bin density map
        std::vector<std::vector<double>> velocityH;  // Horizontal velocity
        std::vector<std::vector<double>> velocityV;  // Vertical velocity
    };
    
    std::vector<std::vector<Bin2D>> bins;
    DensityGrid densityGrid;
    
    void initialize();
    void computeBinCapacities();
    void assignCellsToBins();
    void computeInitialDensity();
    void manipulateDensityMap();  // Equation (8): Adjust density to avoid overspreading
    void computeVelocities(DensityGrid& grid);  // Equation (5): Compute bin velocities
    double interpolateVelocity(double x, double y, bool isHorizontal, const DensityGrid& grid);  // Equation (6)
    void updateCellLocations(const DensityGrid& grid, double deltaT);  // Equation (7)
    void diffusionStep(DensityGrid& grid);  // Equation (4): Update density using FTCS scheme
    void globalDiffusion();  // Algorithm 1
    double calculateDensityOverflow();
};

#endif