# Placement Legalization with Diffusion-Based Migration

A C++ implementation for circuit placement legalization and diffusion-based cell migration. This program reads circuit netlist files in GSRC Bookshelf format, applies density-driven diffusion for cell movement, and performs legalization to place cells on valid row positions.

## Overview

The project consists of four main components:

1. **Parser**: Reads circuit design files in GSRC Bookshelf format (.aux, .nodes, .pl, .nets, .scl, .wts)
2. **Diffusion**: Implements density-driven cell migration using discretized diffusion equations
3. **Legalizer**: Places diffused cells onto valid standard cell rows
4. **Writer**: Outputs modified circuit files in GSRC Bookshelf format

## System Requirements

- **Operating System**: Linux (Ubuntu 18.04 or later recommended)
- **Compiler**: GCC/G++ 7.0 or later (C++17 support required)
- **Build Tool**: GNU Make 4.0 or later
- **Memory**: Minimum 2GB RAM (4GB+ recommended for large circuits)

## Project Structure

```
.
Parser.h            # Circuit file parser header
Parser.cpp          # Circuit file parser implementation
Diffusion.h         # Diffusion algorithm header
Diffusion.cpp       # Diffusion algorithm implementation
Legalizer.h         # Legalization algorithm header
Legalizer.cpp       # Legalization algorithm implementation
Writer.h            # Output file writer header
Writer.cpp          # Output file writer implementation
M11415069.cpp       # Main program entry point
Makefile            # Build configuration
README.md           # This file
```

## Source Code Details

### Parser Component

The `Parser` class handles reading GSRC Bookshelf format files. It supports:

- **AUX files (.aux)**: Entry point containing references to other circuit files
- **NODES files (.nodes)**: Defines cell dimensions (width, height) and types (terminal/movable)
- **PL files (.pl)**: Contains initial cell positions and orientations
- **NETS files (.nets)**: Specifies net connectivity between cells and pins
- **SCL files (.scl)**: Defines standard cell row configurations
- **WTS files (.wts)**: Optional weights for cells and nets

Key methods:
- `parseAll()`: Main entry point that coordinates all sub-parsers
- `parseAux()`: Reads auxiliary file and extracts file references
- `parseNodes()`: Parses cell definitions
- `parsePl()`: Parses placement data and cell positions
- `parseNets()`: Parses net connectivity
- `parseScl()`: Parses row and site configurations
- `parseWts()`: Parses optional cell/net weights
- `getCellOrder()`: Returns cell order from placement file

### Diffusion Component

The `Diffusion` class implements density-driven cell migration using discretized diffusion equations:

- Partitions chip area into 2D bins
- Computes initial cell density distribution
- Applies density manipulation to avoid over-spreading
- Calculates velocity fields based on density gradients
- Updates cell positions iteratively using interpolated velocities
- Uses Forward Time Centered Space (FTCS) numerical scheme

### Legalizer Component

The `Legalizer` class places diffused cells onto valid standard cell rows:

- Partitions circuit into 2D bins for efficient processing
- Implements multi-pass placement strategy with progressive relaxation
- **Pass 1A**: Place within bin boundaries on valid rows
- **Pass 1B**: Place with 簣100% bin width flexibility
- **Pass 2**: Place on nearest row with flexibility
- **Pass 3**: Place on any row without restriction
- Aligns cells to standard cell site grid

### Writer Component

The `Writer` class outputs modified circuit data back to GSRC Bookshelf format:

- **AUX file generation**: Creates new .aux file with correct file references
- **Placement output (.pl)**: Writes updated cell positions after legalization
- **File copying**: Preserves unchanged files (.nodes, .nets, .scl, .wts) by copying

Key methods:
- `writeAll()`: Coordinates output of all circuit files
- `writeAux()`: Creates auxiliary file
- `writePl()`: Writes legalized placement data
- `copyNodes()`, `copyNets()`, `copyScl()`, `copyWts()`: Copy unchanged files
- `setPl()`: Allows custom cell ordering in output

### Main Program (M11415069.cpp)

The main program orchestrates the complete placement legalization workflow:

Features:
- Accepts input .aux file and output prefix as command-line arguments
- Automatically resolves file paths and names
- Stores initial cell positions before diffusion and legalization
- Runs diffusion-based cell migration
- Runs legalization algorithm
- Calculates and reports total and maximum cell displacement
- Writes output files in GSRC Bookshelf format

Key functions:
- `getDirectory()`: Extracts directory path from filepath
- `getBaseName()`: Extracts circuit name from filepath
- Displacement calculation using Manhattan distance metric

## Building the Program

### Step 1: Prepare Source Files

Ensure all source files are present in your project directory:

```
Parser.h, Parser.cpp
Diffusion.h, Diffusion.cpp
Legalizer.h, Legalizer.cpp
Writer.h, Writer.cpp
M11415069.cpp (main program)
Makefile
```

### Step 2: Create Makefile

Create a `Makefile` in your project directory:

```makefile
# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

# Target executable
TARGET = legalizer

# Source files
SOURCES = M11415069.cpp Parser.cpp Diffusion.cpp Legalizer.cpp Writer.cpp
HDRS = Parser.h Diffusion.h Legalizer.h Writer.h
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
```

### Step 3: Compile the Program

```bash
# Navigate to project directory
cd /path/to/project

# Compile the program
make

# Or clean all
make clean

# Verify executable was created
ls -l legalizer
```

### Step 4: Verify Compilation

You should see successful compilation output. Verify the executable:

```bash
file legalizer
# Output: placer: ELF 64-bit LSB executable
```

## Running the Program

### Command-Line Usage

```bash
./legalizer <input.aux> <output_prefix>
```

### Arguments

- **input.aux**: Path to the auxiliary file containing circuit design files
- **output_prefix**: Prefix for output files (will create output_prefix.aux, output_prefix.pl, etc.)

### Examples

**Basic usage with circuit in current directory:**
```bash
./legalizer ibm01.aux legalized_ibm01
```

**With full path to input circuit:**
```bash
./legalizer designs/ibm01/ibm01.aux output/legalized_ibm01
```

**With relative paths:**
```bash
./legalizer ../benchmarks/ibm02/ibm02.aux ../results/legalized_ibm02
```

**In subdirectory:**
```bash
cd /path/to/circuits
../../placer ibm03/ibm03.aux ../../results/legalized_ibm03
```

### Expected Output

The program displays circuit statistics and displacement metrics:

```
Total displacement: 123456.5
Maximum displacement: 2345.6
```

Then exits silently if successful (exit code 0), or displays an error message if something fails.

### Output Files

After successful execution, the following files are created:

- **output_prefix.aux**: Auxiliary file with references to output files
- **output_prefix.pl**: Legalized placement file with updated cell positions
- **output_prefix.nodes**: Copy of original nodes file (unchanged)
- **output_prefix.nets**: Copy of original nets file (unchanged)
- **output_prefix.scl**: Copy of original rows file (unchanged)
- **output_prefix.wts**: Copy of original weights file (unchanged)

## Displacement Metrics

The program calculates and reports two key metrics:

### Total Displacement

Sum of Manhattan distances (L1 norm) for all movable, non-terminal cells:

\[
\text{Total Displacement} = \sum_{i=1}^{n} (|x_i^{\text{final}} - x_i^{\text{initial}}| + |y_i^{\text{final}} - y_i^{\text{initial}}|)
\]

Where n is the number of movable cells.

### Maximum Displacement

Maximum Manhattan distance among all cells:

\[
\text{Max Displacement} = \max_{i=1}^{n} (|x_i^{\text{final}} - x_i^{\text{initial}}| + |y_i^{\text{final}} - y_i^{\text{initial}}|)
\]

## Program Parameters

### Diffusion Parameters (in Diffusion.h)

The `DiffusionParams` structure controls diffusion behavior:

| Parameter | Default | Description |
|-----------|---------|-------------|
| numBinsX | 128 | Number of bins in X direction |
| numBinsY | 128 | Number of bins in Y direction |
| maxIterations | 200 | Maximum diffusion iterations |
| deltaT | 0.05 | Time step size for diffusion |
| targetDensity | 1.0 | Target bin density (0.0-1.0) |

To modify these parameters, edit Diffusion.h and rebuild.

### Legalizer Parameters (in Legalizer.cpp)

| Parameter | Default | Description |
|-----------|---------|-------------|
| numBinsX | 64 | Number of horizontal legalization bins |
| numBinsY | 64 | Number of vertical legalization bins |

## Algorithm Overview

### Phase 1: Parsing

The Parser reads all GSRC Bookshelf files:

1. Reads `.aux` file to identify circuit files
2. Parses `.nodes` for cell definitions (dimensions, types)
3. Parses `.pl` for initial cell positions and orientations
4. Parses `.nets` for net connectivity and pin information
5. Parses `.scl` for row configurations (height, site width, spacing)
6. Parses `.wts` for optional cell and net weights
7. Constructs Circuit data structure

### Phase 2: Initial Position Recording

The main program stores initial cell positions for all movable, non-terminal cells before any modifications. This enables accurate displacement calculation.

### Phase 3: Diffusion-Based Migration

Applies density-driven cell movement:

1. Partitions chip area into 2D bins (default 128*128)
2. Computes initial cell density by calculating intersection areas
3. Applies density manipulation to avoid over-spreading
4. Iteratively:
   - Computes velocity fields from density gradients
   - Interpolates velocities for each cell position
   - Updates cell locations based on velocities
   - Applies FTCS diffusion scheme to density
5. Reduces placement density variations

### Phase 4: Legalization

Places diffused cells onto valid standard cell rows:

1. Partitions circuit into 2D bins (default 64*64)
2. Assigns cells to bins based on current positions
3. Multi-pass cell placement with progressive relaxation
4. Ensures all cells align to standard cell row positions

### Phase 5: Displacement Calculation

Calculates metrics comparing initial and final cell positions:

1. Iterates through all movable, non-terminal cells
2. Computes Manhattan distance for each cell
3. Accumulates total displacement
4. Tracks maximum displacement

### Phase 6: Output Generation

Writer saves results to GSRC Bookshelf format:

1. Generates new `.aux` file with output references
2. Writes legalized placement to `.pl` file
3. Copies unchanged files to preserve circuit integrity


## Troubleshooting

### Compilation Errors

**Error**: `command not found: g++`
- **Solution**: Install GCC compiler
  ```bash
  sudo apt-get install build-essential
  ```

**Error**: `C++17 features not supported`
- **Solution**: Update compiler
  ```bash
  sudo apt-get install g++-9  # Or later version
  # Update Makefile: CXX = g++-9
  ```

**Error**: `undefined reference to Parser::parseAll()`
- **Solution**: Ensure all .cpp files are in Makefile SOURCES
  ```makefile
  SRCS = M11415069.cpp Parser.cpp Diffusion.cpp Legalizer.cpp Writer.cpp
  ```

**Error**: `fatal error: Parser.h: No such file or directory`
- **Solution**: Verify all header files (.h) are in the project directory

### Runtime Errors

**Error**: `Usage: ./legalizer <input.aux> <output_prefix>`
- **Cause**: Incorrect command-line arguments
- **Solution**: Provide exactly two arguments
  ```bash
  ./legalizer input.aux output_prefix
  ```

**Error**: `Cannot open AUX: circuit.aux`
- **Cause**: Input file not found
- **Solution**: Verify file path
  ```bash
  ls -l /path/to/circuit.aux
  ```

**Error**: `Cannot open NODES: circuit.nodes`
- **Cause**: Referenced file missing or incorrect path
- **Solution**: Check all files referenced in .aux exist in same directory

**Error**: `Invalid AUX header`
- **Solution**: Ensure .aux file starts with `RowBasedPlacement :`

**Error**: Segmentation fault
- **Cause**: Insufficient memory or invalid input data
- **Solution**: Reduce bin counts or verify input files are valid

**Error**: `Could not open source file: circuit.nodes` (warning)
- **Cause**: Input file path is incorrect
- **Solution**: Verify all input files are accessible

**Error**: Out of memory
- **Solution**: Reduce parameters for large circuits
  ```h
  diffusionParams.numBinsX = 32;
  diffusionParams.numBinsY = 32;
  ```

### Output Issues

**Problem**: No output files generated
- **Solution**: Check write permissions and disk space
  ```bash
  touch test_file  # Verify write permission
  rm test_file
  df -h  # Check available disk space
  ```

**Problem**: Output files are empty or incomplete
- **Cause**: Program crashed during execution
- **Solution**: Check error messages and verify input files

## Usage Examples

### Basic Placement Legalization

```bash
# Compile
make

# Run on example circuit
./legalizer designs/ibm01/ibm01.aux results/legalized_ibm01
```

### Verifying Results

```bash
# Check output files created
ls legalized_ibm01.*

## Algorithm References

The implementation is based on published research in:

1. **Density-Driven Placement**: Cell migration using diffusion equations
2. **FTCS Scheme**: Forward Time Centered Space numerical discretization
3. **Bilinear Interpolation**: Smooth velocity field calculation
4. **Multi-Pass Legalization**: Progressive relaxation for row placement

## License

This project is provided for educational and research purposes.

## Support

For issues or questions, verify:

1. All source files (.h and .cpp) present in project directory
2. Compiler supports C++17: `g++ --version` (must be 7.0 or later)
3. Input files are in valid GSRC Bookshelf format
4. All referenced files in .aux file exist and are accessible
5. Sufficient disk space for output files
6. Write permissions on output directory
7. Program parameters are within reasonable ranges
8. Sufficient memory available for circuit size
9. No other processes using input files

## Quick Start

1. **Place all source files in project directory**
2. **Run `make` to compile**
3. **Execute: `./legalizer input.aux output_prefix`**
4. **Check output files: `ls output_prefix.*`**

For support or questions, consult this README and verify all prerequisites are met.