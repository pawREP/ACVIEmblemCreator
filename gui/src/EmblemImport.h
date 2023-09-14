#pragma once
#include "Error.h"
#include <filesystem>
#include <vector>

libEmblem::ErrorOr<> importEmblems(const std::filesystem::path& sl2Path, const std::vector<std::filesystem::path>& jsonPath);
libEmblem::ErrorOr<> importEmblems(const std::filesystem::path& sl2Path, const std::filesystem::path& jsonPath);