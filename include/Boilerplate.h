#pragma once

namespace ncnet {
    // Macro functions to remove standard boilerplate code.
    #define BP_SET_GET(var, type)   void set_##var(type var) { var##_ = var; } \
                                    type get_##var() { return var##_; }
    #define BP_GET(var, type)       type get_##var() { return var##_; }
}