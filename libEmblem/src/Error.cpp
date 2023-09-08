#include "Error.h"

Error::Error(std::string_view stringView) : msg(stringView){};

Error::Error(const std::string& str) : msg(str) {
}

const std::string& Error::string() {
    return msg;
};
