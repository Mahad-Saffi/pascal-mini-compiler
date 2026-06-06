#ifndef MINIPASCAL_TYPES_HPP
#define MINIPASCAL_TYPES_HPP

namespace mp {

enum class PType { Integer, Real, Void, Error };

const char* ptypeName(PType type);

}

#endif
