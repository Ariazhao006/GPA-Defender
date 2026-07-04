#pragma once

#include <string>

namespace frontend {

std::string resolveAssetPath(const std::string& relativePath);
bool assetExists(const std::string& relativePath);

}  // namespace frontend
