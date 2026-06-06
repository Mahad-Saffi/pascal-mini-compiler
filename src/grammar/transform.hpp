#ifndef MINIPASCAL_GRAMMAR_TRANSFORM_HPP
#define MINIPASCAL_GRAMMAR_TRANSFORM_HPP

#include "grammar/grammar.hpp"

namespace mp {

Grammar removeLeftRecursion(const Grammar& grammar);
Grammar leftFactor(const Grammar& grammar);
Grammar toTransformed(const Grammar& grammar);

}

#endif
