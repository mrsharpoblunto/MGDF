#pragma once

#include <filesystem>

namespace MGDF {
namespace core {

// Decodes the first frame of an image file into a size x size icon, keeping
// the aspect ratio. Returns nullptr if the file is missing or not an image.
// The caller owns the icon and releases it with DestroyIcon.
HICON LoadIconFromImage(const std::filesystem::path &path, UINT size);

}  // namespace core
}  // namespace MGDF
