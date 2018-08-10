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

std::string bbcPath = "../resources/bbc/tree1/";

/// configure the resource path
void configResPath()
{
	char curDir[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, curDir);
	std::string curPath(curDir);
	resPath = curPath + "\\..\\resources";
}

#endif