#ifndef QUICKJSR_RVECTORVIEW_HPP
#define QUICKJSR_RVECTORVIEW_HPP

#include "quickjs.h"
#include <cpp11.hpp>

namespace quickjsr {
  struct RVectorViewData {
    SEXP source;
    SEXP value;
    bool factor;
    bool date;
    bool mutable_view;
  };

  inline JSClassID js_rvector_view_class_id;
}

#endif
