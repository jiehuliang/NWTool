#include <cstdio>
#include "Util/common.h"
#include "Log/Logging.h"

int main() {
	for (int i = 0; i < 10000; ++i) {
		LOG_INFO << generateRandomInt<uint32_t>() << "\r\n";
	}
}