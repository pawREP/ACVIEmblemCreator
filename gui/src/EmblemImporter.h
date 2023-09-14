#pragma once
#include "Emblem.h"
#include "Error.h"
#include "json.h"
#include <vector>

namespace GeometrizeImporter {

    libEmblem::ErrorOr<std::vector<libEmblem::Image>> fromJson(const nlohmann::json& json);

} // namespace GeometrizeImporter
