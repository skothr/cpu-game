######################################################################
# Automatically generated by qmake (2.01a) Thu Aug 11 17:27:02 2016
######################################################################

# Qt
QT += core gui widgets

# OpenGL
QT += opengl openglextensions

# Program
TARGET = cpugame
TEMPLATE = app debug_and_release

CONFIG += c++1z warn_off debug_and_release
LIBS += -lstdc++fs

QMAKE_CXX_FLAGS_DEBUG += -O0 -pg #-fno-inline-functions
QMAKE_CXX_FLAGS_RELEASE += -O3 -pg #-funroll-all-loops

QMAKE_LDFLAGS_DEBUG += -pg
QMAKE_LDFLAGS_RELEASE += -pg

# sources
# HEADERS += source/inc/gui/gameWidget.hpp source/inc/gui/mainWindow.hpp source/inc/graphics/shader.hpp source/inc/graphics/textureAtlas.hpp source/inc/gui/overlay.hpp source/inc/gui/controlInterface.hpp source/inc/gui/button.hpp source/inc/gui/systemMenu.hpp source/inc/gui/worldCreate.hpp source/inc/gui/worldLoad.hpp source/inc/gui/pauseWidget.hpp source/inc/gui/mainMenu.hpp source/inc/gui/displaySlider.hpp
HEADERS += source/inc/compute/*.hpp source/inc/graphics/*.hpp source/inc/gui/*.hpp source/inc/math/*.hpp source/inc/threading/*.hpp source/inc/tools/*.hpp source/inc/voxels/*.hpp ./libs/FastNoise/*.h
SOURCES += source/src/compute/*.cpp source/src/graphics/*.cpp source/src/gui/*.cpp source/src/math/*.cpp source/src/threading/*.cpp source/src/tools/*.cpp source/src/voxels/*.cpp source/src/*.cpp ./libs/FastNoise/*.cpp

#SOURCES += libs/FastNoise/*.cpp
#SOURCES += libs/tinyobjloader/tiny_obj_loader.cc

# Paths
DEPENDPATH = ./source/src/compute ./source/src/graphics ./source/src/gui ./source/src/math ./source/src/threading ./source/src/tools ./source/src/voxels ./source/src ./libs/FastNoise ./libs/FastNoiseSIMD/FastNoiseSIMD
INCLUDEPATH = ./config ./source/inc/compute ./source/inc/graphics ./source/inc/gui ./source/inc/math ./source/inc/threading ./source/inc/tools ./source/inc/voxels ./source/inc ./libs/FastNoise ./libs/FastNoiseSIMD/FastNoiseSIMD

OBJECTS_DIR = build/.obj
MOC_DIR = build/.moc
RCC_DIR = build/.rcc
UI_DIR = build/.ui


# CONFIG(debug, debug|release) {
#     OBJECTS_DIR = build/debug/.obj
#     MOC_DIR = build/debug/.moc
#     RCC_DIR = build/debug/.rcc
#     UI_DIR = build/debug/.ui
# }
# CONFIG(release, debug|release) {
#     OBJECTS_DIR = build/release/.obj
#     MOC_DIR = build/release/.moc
#     RCC_DIR = build/release/.rcc
#     UI_DIR = build/release/.ui
# }
