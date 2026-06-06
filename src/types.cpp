#include "types.hpp"

namespace mp {

const char* ptypeName(PType type) {
    switch (type) {
        case PType::Integer: return "integer";
        case PType::Real:    return "real";
        case PType::Void:    return "void";
        case PType::Error:   return "error";
    }
    return "error";
}

}
