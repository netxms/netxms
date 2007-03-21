/*
 * symbols.cxx
 *
 * libtpt 1.30
 *
 */

/*
 * Copyright (C) 2002-2004 Isaac W. Foraker (isaac@tazthecat.net)
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "libtpt_conf.h"
#include "libtpt_symbols_impl.h"
#include "libtpt_vars.h"       // Default variables
#include <libtpt/symbols.h>
#include <libtpt/parse.h>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace TPT {

/**
 * Construct an instance of the Symbols class.
 *
 * @param   setdefaults     When true, set the default variables
 * (default true).
 * @return  nothing
 */
Symbols::Symbols(bool setdefaults)
{
    imp = new Symbols_Impl(*this);
    if (setdefaults)
        for (unsigned i=0; i<numbuiltins; ++i)
            imp->symbols.hash()[libtpt_builtins[i].id] =
                new Object(libtpt_builtins[i].value);
}


/**
 * Construct a copy of the specified Symbols class.
 *
 * @param   s               The Symbols table to copy.
 * @return  nothing
 */
Symbols::Symbols(const Symbols& s)
{
    imp->copy(s.imp->symbols);
}


/**
 * Destruct this instance of the Symbols class.
 *
 * @return  nothing
 */
Symbols::~Symbols()
{
    delete imp;
}


/**
 * Copy the specified symbols table.  This differs from the operator= copy
 * function in that it does not remove existing symbols unless the symbol also
 * exists in the source symbols table.
 *
 * @param   sym     The source symbols table.
 * @return  nothing
 */
void Symbols::copy(const Symbols& sym)
{
    imp->copy(const_cast<Object&>(sym.imp->symbols));
}


/**
 * Symbols copy operator
 *
 * @param   sym     A Symbols table to copy.
 * @return  Reference to *this instance of Symbols.
 */
Symbols& Symbols::operator=(const Symbols& sym)
{
    imp->symbols = sym.imp->symbols;
    return *this;
}


/**
 * Get the value specified by id from the symbols table, recursing to
 * process embedded symbols as needed.
 *
 * @param   id          ID of symbol to retrieve.
 * @param   outval      Symbol array to receive value(s).
 * @return  false on success;
 * @return  true if symbol is invalid.
 */
bool Symbols::get(const SymbolKeyType& id, SymbolValueType& outval) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
    {
        outval.erase();
        return true;
    }
    if (!pobj.get())
        return true;
    switch (pobj->gettype()) {
    case Object::type_scalar:
        outval = pobj->scalar();
        break;
    case Object::type_array:
        outval = "[ARRAY]";
        break;
    case Object::type_hash:
        outval = "[HASH]";
        break;
    case Object::type_notalloc:
        // This should never be reached!
        outval = "[ERROR]";
        break;
    default:
        outval.erase();
        return true;
    }
    return false;
}

/**
 * Get the array of values specified by id from the symbols table,
 * recursing to process embedded symbols as needed.
 *
 * @param   id          ID of symbol to retrieve.
 * @param   outval      Symbol array to receive value(s).
 * @return  false on success;
 * @return  true if symbol is invalid.
 */
bool Symbols::get(const SymbolKeyType& id, SymbolArrayType& outval) const
{
    Object::PtrType pobj;
    outval.clear();
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return true;
    switch (pobj->gettype()) {
    case Object::type_scalar:
        outval.push_back(pobj->scalar());
        break;
    case Object::type_array:
        {
            Object::ArrayType array(pobj->array());
            Object::ArrayType::iterator it(array.begin()),
                end(array.end());
            for (; it != end; ++it) {
                Object& aobj = *(*it);
                if (aobj.gettype() == Object::type_scalar)
                    outval.push_back(aobj.scalar());
                // else ignore
            }
        }
        break;
    case Object::type_hash:
        outval.push_back("[HASH]");
        break;
    default:
        outval.clear();
        return true;
    }
    return false;
}


/**
 * Set a symbol's value.
 *
 * @param   id          ID of the scalar to be set.
 * @param   value       String to be copied.
 * @return  false on success;
 * @return  true if invalid id.
 */
bool Symbols::set(const SymbolKeyType& id, const SymbolValueType& value)
{
    return imp->setobject(id, value, imp->symbols);
}


/**
 * Set a symbol's value.
 *
 * @param   id          ID of the scalar to be set.
 * @param   value       Integer to be copied.
 * @return  false on success;
 * @return  true if invalid id.
 */
bool Symbols::set(const SymbolKeyType& id, int value)
{
    char temp[64];
    std::sprintf(temp, "%d", value);
    return imp->setobject(id, temp, imp->symbols);
}


/**
 * Copy an array of strings to the specified symbol.
 *
 * @param   id          ID of the array to be set.
 * @param   value       Array values to be copied.
 * @return  nothing
 */
bool Symbols::set(const SymbolKeyType& id, const SymbolArrayType& value)
{
    return imp->setobject(id, value, imp->symbols);
}


/**
 * Copy a string hash to the specified symbol.
 *
 * @param   id          ID of the hash to be set.
 * @param   value       Hash to be copied.
 * @return  nothing
 */
bool Symbols::set(const SymbolKeyType& id, const SymbolHashType& value)
{
    return imp->setobject(id, value, imp->symbols);
}


/**
 * Assign an Object into the specified symbol.
 *
 * @param   id          ID of symbol to set.
 * @param   value       Reference to Object to assign.
 * @return  nothing
 */
bool Symbols::set(const SymbolKeyType& id, Object& value)
{
    return imp->setobject(id, value, imp->symbols);
}


/**
 * Push a symbol's value
 *
 * @param   id          ID of the array to receive string.
 * @param   value       Value of the symbol to be pushed.
 * @return  false on success;
 * @return  true if invalid id.
 */
bool Symbols::push(const SymbolKeyType& id, const SymbolValueType& value)
{
    return imp->pushobject(id, value, imp->symbols);
}


/**
 * Push a symbol's array values
 *
 * @param   id          ID of the array to receive array.
 * @param   value       Array values of the symbol to be pushed.
 * @return  nothing
 */
bool Symbols::push(const SymbolKeyType& id, const SymbolArrayType& value)
{
    return imp->pushobject(id, value, imp->symbols);
}


/**
 * Push a symbol's hash values
 *
 * @param   id          ID of the array to receive hash.
 * @param   value       Array values of the symbol to be pushed.
 * @return  nothing
 */
bool Symbols::push(const SymbolKeyType& id, const SymbolHashType& value)
{
    return imp->pushobject(id, value, imp->symbols);
}


/**
 * Push a symbol's hash values
 *
 * @param   id          ID of the array to receive object.
 * @param   value       Object to be pushed onto array.
 * @return  nothing
 */
bool Symbols::push(const SymbolKeyType& id, Object& value)
{
    return imp->pushobject(id, value, imp->symbols);
}


/**
 * Check whether the specified id exists in the symbol table.
 *
 * @param   id          ID of symbol to retrieve.
 * @return  true if symbol exists;
 * @return  false if symbol does not exist, or id invalid.
 */
bool Symbols::exists(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    return !imp->getobjectforget(id, imp->symbols, pobj);
}


/**
 * Check if a symbol is empty.
 *
 * @param   id          ID of symbol to retrieve.
 * @return  true if symbol is empty;
 * @return  false if symbol is not empty.
 */
bool Symbols::empty(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return false;
    switch (pobj->gettype()) {
    case Object::type_scalar:
        return pobj->scalar().empty();
    case Object::type_hash:
        return pobj->hash().empty();
    case Object::type_array:
        return pobj->array().empty();
    default:
        break;
    }
    return true;
}


/**
 * Check if a symbol is a multielement scalar.
 *
 * @param   id      ID of the symbol.
 * @return  true if symbols is an scalar;
 * @return  false if symbol is not an scalar, or does not exist;
 */
bool Symbols::isscalar(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return false;
    return pobj->gettype() == Object::type_scalar;
}


/**
 * Check if a symbol is a multielement array.
 *
 * @param   id      ID of the symbol.
 * @return  true if symbols is an array;
 * @return  false if symbol is not an array, or does not exist;
 */
bool Symbols::isarray(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return false;
    return pobj->gettype() == Object::type_array;
}


/**
 * Check if a symbol is a hash.
 *
 * @param   id      ID of the symbol.
 * @return  true if symbols is an hash;
 * @return  false if symbol is not an hash, or does not exist;
 */
bool Symbols::ishash(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return false;
    return pobj->gettype() == Object::type_hash;
}


/**
 * Get size of the specified array.
 *
 * @param   id      ID of the array.
 * @return  size of the array;
 * @return  0 if array is empty, symbol is not an array, or symbold
 * does not exist;
 */
unsigned Symbols::size(const SymbolKeyType& id) const
{
    Object::PtrType pobj;
    if (imp->getobjectforget(id, imp->symbols, pobj))
        return 0;
    if (pobj->gettype() == Object::type_array)
        return pobj->array().size();
    else
        return 0;
}


/**
 * Unset a symbols
 *
 * @param   id      ID of the symbol to clear.
 * @return  true if symbol unset;
 * @return  false if symbol does not exist, or id invalid.
 */
bool Symbols::unset(const SymbolKeyType& id)
{
    return imp->setobject(id, "", imp->symbols);
}


} // end namespace TPT
