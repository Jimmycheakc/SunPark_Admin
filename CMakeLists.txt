cmake_minimum_required(VERSION 3.12)

project(LinuxPBS)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Set the root directory for the target environment
set(CMAKE_FIND_ROOT_PATH /home/sunpark/workspace/x7v2_20222_D/sysroots/cortexa72-cortexa53-xilinx-linux)

# Specify the sysroot directory
set(CMAKE_SYSROOT /home/sunpark/workspace/x7v2_20222_D/sysroots/cortexa72-cortexa53-xilinx-linux)
# Add include directories
include_directories(/home/sunpark/workspace/x7v2_20222_D/sysroots/cortexa72-cortexa53-xilinx-linux/usr/local/include)

# Add link directories
link_directories(/home/sunpark/workspace/x7v2_20222_D/sysroots/cortexa72-cortexa53-xilinx-linux/usr/local/lib)
link_directories(/home/sunpark/workspace/x7v2_20222_D/sysroots/cortexa72-cortexa53-xilinx-linux/usr/lib)

# Add include directories
# include_directories(/usr/local/include)
# include_directories(/usr/local/include/spdlog)
# include_directories(/usr/local/include/boost)

# Add link directories
# link_directories(/usr/local/lib)
# link_directories(/usr/lib)

# Lisy your source files
set(SOURCE_FILES
    ini_parser.cpp
    ce_time.cpp
    main.cpp
)

# Create an executable
add_executable(admin ${SOURCE_FILES})

# Link against libraries
target_link_libraries(admin)