/*
 * eval.h
 *
 * libtpt 1.30
 *
 */

#include <libtpt/symbols.h>

#include <string>

namespace TPT {

std::string eval(const std::string& expr, const Symbols* sym=0);


} // end namespace TPT
