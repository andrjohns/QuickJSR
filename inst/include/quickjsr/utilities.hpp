#ifndef QUICKJSR_UTILITIES_HPP
#define QUICKJSR_UTILITIES_HPP

#include <cpp11.hpp>

namespace quickjsr {

const char* to_cstring(const SEXP& str, int64_t idx = 0) {
  return Rf_translateCharUTF8(STRING_ELT(str, idx));
}

} // namespace quickjsr

#endif
