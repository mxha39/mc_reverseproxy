# Variables
SRC_DIR = src
HEADER_DIR = $(SRC_DIR)/header
BUILD_DIR = build
EXECUTABLE = $(BUILD_DIR)/mc_reverseproxy
MAIN_FILE = $(SRC_DIR)/main.cpp

# Find all .cpp files in the source directory
CPP_FILES = $(wildcard $(SRC_DIR)/*.cpp)

# Find all corresponding .h files in the header directory
HEADER_FILES = $(wildcard $(HEADER_DIR)/*.h)

# Extract base names of header files (without directory and extension)
HEADER_BASE_NAMES = $(basename $(notdir $(HEADER_FILES)))

# Filter .cpp files to only include those with corresponding headers, plus main.cpp
SRC_FILES = $(filter $(SRC_DIR)/%, \
	$(MAIN_FILE) \
	$(foreach file, $(CPP_FILES), \
		$(if $(filter $(basename $(notdir $(file))), $(HEADER_BASE_NAMES)), $(file))))

# Rule to build the executable
$(EXECUTABLE): $(SRC_FILES)
	mkdir -p $(BUILD_DIR)
	g++ $(SRC_FILES) -o $(EXECUTABLE) -lresolv

# Phony target to clean the build directory
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)


.PHONY: run
run: $(EXECUTABLE)
	clear
	./$(EXECUTABLE)