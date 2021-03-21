#ifndef CONFIG_H
#define CONFIG_H

#include "camera.h"
#include <windows.h>
#include <iostream>

/// image&srceen width height
unsigned int IMG_WIDTH = 1024;
unsigned int IMG_HEIGHT = 1024;
unsigned int SRC_WIDTH = 1000;
unsigned int SRC_HEIGHT = 600;

/// resource path
std::string resPath;


#define MAX_PATH_LEN 256
std::string bbcPath = "..\\assets\\bbc\\tree1\\";

#endif