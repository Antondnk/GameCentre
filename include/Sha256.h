#pragma once

#include <string>

namespace gc {

// SHA-256 в шестнадцатеричном виде (используется для хэширования паролей).
std::string sha256_hex(const std::string& data);

}  // namespace gc
