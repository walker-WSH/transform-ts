#include <iostream>
#include "transform-systime.h"

int main()
{
	while (1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
}
