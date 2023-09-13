#pragma once
#include "Emblem.h"
#include "Error.h"
#include "json.h"
#include <vector>

namespace GeometrizeImporter {

    ErrorOr<std::vector<Image>> fromJson(const nlohmann::json& json);

} // namespace GeometrizeImporter
