#include <iostream>
#include "config.h"



using namespace std;

int main(int argc, char *argv[]) {
    std::cout << "start test:" << std::endl;
	MarketRobot::CConfig::instance().writeConfig();
    system("pause");
}
