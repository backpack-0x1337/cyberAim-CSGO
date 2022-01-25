#include <assert.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>

namespace hwid {
    std::string getDiskHWID();
    bool isUserHWIDValid();
}