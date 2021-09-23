#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// i really have to do all this just to share a function between two files
// i was going to just copy and paste the function between the two files but my conscience wouldn't let me
// and i cant just define the whole function in the header file because then it's all like "ooo you double include"
std::string getNextValidLine(std::ifstream& in);