# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.8

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/local/Cellar/cmake/3.8.2/bin/cmake

# The command to remove a file.
RM = /usr/local/Cellar/cmake/3.8.2/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/alchemystar/mycode/hero

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/alchemystar/mycode/hero/build

# Include any dependencies generated for this target.
include CMakeFiles/hero.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/hero.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/hero.dir/flags.make

CMakeFiles/hero.dir/main.c.o: CMakeFiles/hero.dir/flags.make
CMakeFiles/hero.dir/main.c.o: ../main.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/alchemystar/mycode/hero/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/hero.dir/main.c.o"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/hero.dir/main.c.o   -c /Users/alchemystar/mycode/hero/main.c

CMakeFiles/hero.dir/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/hero.dir/main.c.i"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /Users/alchemystar/mycode/hero/main.c > CMakeFiles/hero.dir/main.c.i

CMakeFiles/hero.dir/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/hero.dir/main.c.s"
	/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /Users/alchemystar/mycode/hero/main.c -o CMakeFiles/hero.dir/main.c.s

CMakeFiles/hero.dir/main.c.o.requires:

.PHONY : CMakeFiles/hero.dir/main.c.o.requires

CMakeFiles/hero.dir/main.c.o.provides: CMakeFiles/hero.dir/main.c.o.requires
	$(MAKE) -f CMakeFiles/hero.dir/build.make CMakeFiles/hero.dir/main.c.o.provides.build
.PHONY : CMakeFiles/hero.dir/main.c.o.provides

CMakeFiles/hero.dir/main.c.o.provides.build: CMakeFiles/hero.dir/main.c.o


# Object files for target hero
hero_OBJECTS = \
"CMakeFiles/hero.dir/main.c.o"

# External object files for target hero
hero_EXTERNAL_OBJECTS =

hero: CMakeFiles/hero.dir/main.c.o
hero: CMakeFiles/hero.dir/build.make
hero: net/libnet.a
hero: net/proto/libproto.a
hero: CMakeFiles/hero.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/alchemystar/mycode/hero/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C executable hero"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/hero.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/hero.dir/build: hero

.PHONY : CMakeFiles/hero.dir/build

CMakeFiles/hero.dir/requires: CMakeFiles/hero.dir/main.c.o.requires

.PHONY : CMakeFiles/hero.dir/requires

CMakeFiles/hero.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/hero.dir/cmake_clean.cmake
.PHONY : CMakeFiles/hero.dir/clean

CMakeFiles/hero.dir/depend:
	cd /Users/alchemystar/mycode/hero/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/alchemystar/mycode/hero /Users/alchemystar/mycode/hero /Users/alchemystar/mycode/hero/build /Users/alchemystar/mycode/hero/build /Users/alchemystar/mycode/hero/build/CMakeFiles/hero.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/hero.dir/depend
