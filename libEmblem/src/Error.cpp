#include "Error.h"

libEmblem::Error::Error(std::string_view stringView) : msg(stringView){};

libEmblem::Error::Error(const std::string& str) : msg(str) {
}

const std::string& libEmblem::Error::string() {
    return msg;
};
