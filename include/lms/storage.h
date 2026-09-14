#pragma once

#include <string>

#include "lms/library.h"
#include "lms/result.h"

namespace lms {

// Persistence (feature: data survives restarts). Plain text, one record per
// line, tab-separated, so the file is diff-friendly and needs no third-party
// JSON library. Format is documented in docs/FEATURES.md.
std::string serialize(const Library& library);
Result deserialize(const std::string& text, Library& library);

Result saveToFile(const Library& library, const std::string& path);
// A missing file is not an error: the library simply starts empty.
Result loadFromFile(Library& library, const std::string& path);

// Fills an empty library with the default catalogue (replaces the original
// 15 "Title by Author" placeholders with real books).
void seedDefaultCatalogue(Library& library);

}  // namespace lms
