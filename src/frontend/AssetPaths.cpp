#include "frontend/AssetPaths.h"

#include "raylib.h"

#include <filesystem>
#include <vector>

namespace frontend {
namespace {

namespace fs = std::filesystem;

void addRoot(std::vector<fs::path>& roots, const fs::path& root) {
    if (root.empty()) return;

    std::error_code ec;
    fs::path normalized = fs::absolute(root, ec).lexically_normal();
    if (ec) return;

    for (const fs::path& existing : roots) {
        if (existing == normalized) return;
    }
    roots.push_back(normalized);
}

std::vector<fs::path> candidateRoots() {
    std::vector<fs::path> roots;

    addRoot(roots, GetWorkingDirectory());

    fs::path appDir = GetApplicationDirectory();
    addRoot(roots, appDir);
    addRoot(roots, appDir / "..");
    addRoot(roots, appDir / "../..");

#ifdef GPA_DEFENDER_PROJECT_ROOT
    addRoot(roots, GPA_DEFENDER_PROJECT_ROOT);
#endif

    return roots;
}

bool findAsset(const std::string& relativePath, fs::path& outPath) {
    fs::path requested(relativePath);

    std::error_code ec;
    if (requested.is_absolute() && fs::exists(requested, ec)) {
        outPath = requested.lexically_normal();
        return true;
    }

    for (const fs::path& root : candidateRoots()) {
        fs::path candidate = (root / requested).lexically_normal();
        if (fs::exists(candidate, ec)) {
            outPath = candidate;
            return true;
        }
    }

    return false;
}

}  // namespace

std::string resolveAssetPath(const std::string& relativePath) {
    fs::path path;
    if (findAsset(relativePath, path)) {
        return path.generic_string();
    }

    TraceLog(LOG_WARNING, "Asset not found: %s", relativePath.c_str());
    return relativePath;
}

bool assetExists(const std::string& relativePath) {
    fs::path path;
    return findAsset(relativePath, path);
}

}  // namespace frontend
