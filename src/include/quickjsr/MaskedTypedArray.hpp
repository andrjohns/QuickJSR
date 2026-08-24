#ifndef QUICKJSR_MASKEDTYPEDARRAY_HPP
#define QUICKJSR_MASKEDTYPEDARRAY_HPP

#include "quickjs.h"
#include <cpp11.hpp>

namespace quickjsr {
  struct MaskedTypedArrayData {
    SEXPTYPE type;
  };

  inline JSClassID js_masked_typed_array_class_id;
}

#endif
