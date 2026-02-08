/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: env.cpp
**
**/

#include "libnxsl.h"
#include <netxms-version.h>

/**
 * Externals
 */
int F_assert(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_chr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_index(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_inList(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_left(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_length(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_mktime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ord(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_print(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_println(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_range(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_replace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_right(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_sleep(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_x2d(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ArrayToString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ByteStream(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm);
int F_DateTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FormatMetricPrefix(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm);
int F_FormatNumber(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GeoLocation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetCurrentTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetCurrentTimeMs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetThreadPoolNames(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_InetAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonArray(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonObject(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_JsonParse(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_LevenshteinDistance(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MacAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GetMonotonicClockTime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ReadPersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SimilarityScore(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SplitString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Table(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_WritePersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_Base64Decode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_Base64Encode(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_CryptoMD5(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_CryptoSHA1(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_CryptoSHA256(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_MathAbs(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAcos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAsin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAtan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAtan2(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAtanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathAverage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathCeil(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathCos(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathCosh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathExp(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathFloor(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathLog(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathLog10(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathMax(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathMeanAbsoluteDeviation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathMin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathPow(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathPow10(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathRandom(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathRound(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathSin(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathSinh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathStandardDeviation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathSqrt(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathSum(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathTan(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathTanh(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_MathWeierstrass(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_NetResolveAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_NetResolveHostname(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_NetGetLocalHostName(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_CopyFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_CreateDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_DeleteFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FileAccess(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_OpenFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_RemoveDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_RenameFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int S_max(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);
int S_min(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);

/**
 * Placeholder for __invoke function - will only be called if no arguments provided or via indirect call
 */
static int F_invoke(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   return (argc == 0) ? NXSL_ERR_INVALID_ARGUMENT_COUNT : NXSL_ERR_TOO_MANY_NESTED_CALLS;
}

/**
 * Default built-in function list
 */
static NXSL_ExtFunction s_builtinFunctions[] =
{
   { "__invoke", F_invoke, -1 },
   { "__new@ByteStream", F_ByteStream, 0 },
   { "__new@DateTime", F_DateTime, -1 },
   { "__new@GeoLocation", F_GeoLocation, -1 },
   { "__new@InetAddress", F_InetAddress, -1 },
   { "__new@JsonArray", F_JsonArray, -1 },
   { "__new@JsonObject", F_JsonObject, -1 },
   { "__new@MacAddress", F_MacAddress, -1 },
   { "__new@Range", F_range, -1 },
   { "__new@Table", F_Table, 0 },
	{ "_exit", F_exit, -1 },
   { "abs", F_MathAbs, 1, true },
   { "assert", F_assert, -1 },
   { "ceil", F_MathCeil, 1, true },
   { "chr", F_chr, 1 },
   { "classof", F_classof, 1 },
   { "cos", F_MathCos, 1, true },
	{ "d2x", F_d2x, -1 },
   { "floor", F_MathFloor, 1, true },
   { "format", F_FormatNumber, -1, true },
   { "gmtime", F_gmtime, -1, true },
   { "index", F_index, -1, true },
   { "inList", F_inList, 3, true },
   { "left", F_left, -1, true },
   { "length", F_length, 1, true },
   { "localtime", F_localtime, -1, true },
   { "lower", F_lower, 1, true },
	{ "ltrim", F_ltrim, 1, true },
   { "max", F_MathMax, -1, true },
   { "min", F_MathMin, -1, true },
   { "mktime", F_mktime, 1, true },
   { "ord", F_ord, 1 },
   { "pow", F_MathPow, 2, true },
   { "print", F_print, -1 },
   { "println", F_println, -1 },
   { "random", F_MathRandom, 2, true },
   { "range", F_range, -1 },
   { "replace", F_replace, 3, true },
   { "right", F_right, -1, true },
   { "rindex", F_rindex, -1, true },
   { "round", F_MathRound, -1, true },
	{ "rtrim", F_rtrim, 1, true },
   { "sin", F_MathSin, 1, true },
	{ "sleep", F_sleep, 1 },
   { "sqrt", F_MathSqrt, 1, true },
	{ "strftime", F_strftime, -1, true },
	{ "substr", F_substr, -1, true },
   { "tan", F_MathTan, 1, true },
	{ "time", F_GetCurrentTime, 0 },
   { "trace", F_trace, 2 },
	{ "trim", F_trim, 1, true },
   { "typeof", F_typeof, 1 },
   { "upper", F_upper, 1, true },
   { "x2d", F_x2d, 1 },
   { "AddrInRange", F_AddrInRange, 3, true },
   { "AddrInSubnet", F_AddrInSubnet, 3, true },
   { "ArrayToString", F_ArrayToString, 2, true },
   { "Base64Decode", F_Base64Decode, -1, true },
   { "Base64Encode", F_Base64Encode, -1, true },
   { "FormatMetricPrefix", F_FormatMetricPrefix, -1 },
   { "FormatNumber", F_FormatNumber, -1 },
   { "GetCurrentTime", F_GetCurrentTime, 0 },
   { "GetCurrentTimeMs", F_GetCurrentTimeMs, 0 },
   { "GetMonotonicClockTime", F_GetMonotonicClockTime, 0 },
   { "GetThreadPoolNames", F_GetThreadPoolNames, 0 },
   { "JsonParse", F_JsonParse, -1 },
   { "LevenshteinDistance", F_LevenshteinDistance, -1 },
   { "ReadPersistentStorage", F_ReadPersistentStorage, 1 },
	{ "SecondsToUptime", F_SecondsToUptime, 1 },
   { "SimilarityScore", F_SimilarityScore, -1 },
   { "SplitString", F_SplitString, 2, true },
   { "WritePersistentStorage", F_WritePersistentStorage, 2 }
};

/**
 * Default built-in selector list
 */
static NXSL_ExtSelector s_builtinSelectors[] =
{
   { "max", S_max },
   { "min", S_min }
};

/**
 * Base64:: functions
 */
static NXSL_ExtFunction s_base64Functions[] =
{
   { "Decode", F_Base64Decode, -1 },
   { "Encode", F_Base64Encode, -1 }
};

/**
 * Crypto:: functions
 */
static NXSL_ExtFunction s_cryptoFunctions[] =
{
   { "MD5", F_CryptoMD5, 1 },
   { "SHA1", F_CryptoSHA1, 1 },
   { "SHA256", F_CryptoSHA256, 1 }
};

/**
 * Math:: functions
 */
static NXSL_ExtFunction s_mathFunctions[] =
{
   { "Abs", F_MathAbs, 1 },
   { "Acos", F_MathAcos, 1 },
   { "Asin", F_MathAsin, 1 },
   { "Atan", F_MathAtan, 1 },
   { "Atan2", F_MathAtan2, 2 },
   { "Atanh", F_MathAtanh, 1 },
   { "Average", F_MathAverage, -1 },
   { "Ceil", F_MathCeil, 1 },
   { "Cos", F_MathCos, 1 },
   { "Cosh", F_MathCosh, 1 },
   { "Exp", F_MathExp, 1 },
   { "Floor", F_MathFloor, 1 },
   { "Log", F_MathLog, 1 },
   { "Log10", F_MathLog10, 1 },
   { "Max", F_MathMax, -1 },
   { "MeanAbsoluteDeviation", F_MathMeanAbsoluteDeviation, -1 },
   { "Min", F_MathMin, -1 },
   { "Pow", F_MathPow, 2 },
   { "Pow10", F_MathPow10, 1 },
   { "Random", F_MathRandom, 2 },
   { "Round", F_MathRound, -1 },
   { "Sin", F_MathSin, 1 },
   { "Sinh", F_MathSinh, 1 },
   { "StandardDeviation", F_MathStandardDeviation, -1 },
   { "Sqrt", F_MathSqrt, 1 },
   { "Sum", F_MathSum, -1 },
   { "Tan", F_MathTan, 1 },
   { "Tanh", F_MathTanh, 1 },
   { "Weierstrass", F_MathWeierstrass, 3 }
};

/**
 * Net:: functions
 */
static NXSL_ExtFunction s_netFunctions[] =
{
   { "GetLocalHostName", F_NetGetLocalHostName, -1 },
   { "ResolveAddress", F_NetResolveAddress, -1 },
   { "ResolveHostname", F_NetResolveHostname, 1 }
};

/**
 * Default built-in modules
 */
static NXSL_ExtModule s_builtinModules[] =
{
   { "Base64", sizeof(s_base64Functions) / sizeof(NXSL_ExtFunction), s_base64Functions },
   { "Crypto", sizeof(s_cryptoFunctions) / sizeof(NXSL_ExtFunction), s_cryptoFunctions },
   { "Math", sizeof(s_mathFunctions) / sizeof(NXSL_ExtFunction), s_mathFunctions },
   { "Net", sizeof(s_netFunctions) / sizeof(NXSL_ExtFunction), s_netFunctions }
};

/**
 * Constructor
 */
NXSL_Environment::NXSL_Environment() : m_metadata(1024)
{
   m_functions = createFunctionListRef(s_builtinFunctions, sizeof(s_builtinFunctions) / sizeof(NXSL_ExtFunction));
   m_selectors = createSelectorListRef(s_builtinSelectors, sizeof(s_builtinSelectors) / sizeof(NXSL_ExtSelector));
   m_modules = createModuleListRef(s_builtinModules, sizeof(s_builtinModules) / sizeof(NXSL_ExtModule));
   m_library = nullptr;
}

/**
 * Destructor
 */
NXSL_Environment::~NXSL_Environment()
{
}

/**
 * Find function by name
 */
const NXSL_ExtFunction *NXSL_Environment::findFunction(const NXSL_Identifier& name) const
{
   for(NXSL_EnvironmentListRef<NXSL_ExtFunction> *list = m_functions; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
         if (name.equals(list->elements[i].m_name))
            return &list->elements[i];
   }
   return nullptr;
}

/**
 * Get all available functions
 */
StringSet *NXSL_Environment::getAllFunctions() const
{
   StringSet *functions = new StringSet();
   for(NXSL_EnvironmentListRef<NXSL_ExtFunction> *list = m_functions; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
      {
#ifdef UNICODE
         functions->addPreallocated(WideStringFromUTF8String(list->elements[i].m_name.value));
#else
         functions->add(list->elements[i].m_name.value);
#endif
      }
   }

   for(NXSL_EnvironmentListRef<NXSL_ExtModule> *list = m_modules; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
      {
         const NXSL_ExtModule& module = list->elements[i];

         size_t noffset = module.m_name.length;
         char fname[256];
         memcpy(fname, module.m_name.value, noffset);
         fname[noffset++] = ':';
         fname[noffset++] = ':';

         for(int j = 0; j < module.m_numFunctions; j++)
         {
            strcpy(&fname[noffset], module.m_functions[j].m_name.value);
#ifdef UNICODE
            functions->addPreallocated(WideStringFromUTF8String(fname));
#else
            functions->add(fname);
#endif
         }
      }
   }

   return functions;
}

/**
 * Register function set
 */
void NXSL_Environment::registerFunctionSet(size_t count, const NXSL_ExtFunction *list)
{
   NXSL_EnvironmentListRef<NXSL_ExtFunction> *ref = createFunctionListRef(list, count);
   ref->next = m_functions;
   m_functions = ref;
}

/**
 * I/O and file management functions
 */
static NXSL_ExtFunction s_ioFunctions[] =
{
   { "CopyFile", F_CopyFile, 2 },
   { "CreateDirectory", F_CreateDirectory, 1 },
   { "DeleteFile", F_DeleteFile, 1 },
   { "FileAccess", F_FileAccess, 2 },
   { "OpenFile", F_OpenFile, -1 },
   { "RemoveDirectory", F_RemoveDirectory, 1 },
   { "RenameFile", F_RenameFile, 2 }
};
static NXSL_ExtModule s_ioModule = { "IO", (int)(sizeof(s_ioFunctions) / sizeof(NXSL_ExtFunction)), s_ioFunctions };

/**
 * Register I/O and file management functions
 */
void NXSL_Environment::registerIOFunctions()
{
   registerModuleSet(1, &s_ioModule);
}

/**
 * Find external module by name
 */
const NXSL_ExtModule *NXSL_Environment::findModule(const NXSL_Identifier& name) const
{
   for(NXSL_EnvironmentListRef<NXSL_ExtModule> *list = m_modules; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
         if (name.equals(list->elements[i].m_name))
            return &list->elements[i];
   }
   return nullptr;
}

/**
 * Register external module set
 */
void NXSL_Environment::registerModuleSet(size_t count, const NXSL_ExtModule *list)
{
   NXSL_EnvironmentListRef<NXSL_ExtModule> *ref = createModuleListRef(list, count);
   ref->next = m_modules;
   m_modules = ref;
}

/**
 * Find selector by name
 */
const NXSL_ExtSelector *NXSL_Environment::findSelector(const NXSL_Identifier& name) const
{
   for(NXSL_EnvironmentListRef<NXSL_ExtSelector> *list = m_selectors; list != nullptr; list = list->next)
   {
      for(size_t i = 0; i < list->count; i++)
         if (!strcmp(list->elements[i].m_name, name.value))
            return &list->elements[i];
   }
   return nullptr;
}

/**
 * Register selector set
 */
void NXSL_Environment::registerSelectorSet(size_t count, const NXSL_ExtSelector *list)
{
   NXSL_EnvironmentListRef<NXSL_ExtSelector> *ref = createSelectorListRef(list, count);
   ref->next = m_selectors;
   m_selectors = ref;
}

/**
 * Load module into VM
 */
bool NXSL_Environment::loadModule(NXSL_VM *vm, const NXSL_ModuleImport *importInfo)
{
   bool success = false;

   // Check registered external modules
   if (!(importInfo->flags & MODULE_IMPORT_WILDCARD))
   {
      const NXSL_ExtModule *module = findModule(importInfo->name);
      if (module != nullptr)
      {
         success = vm->loadExternalModule(module, importInfo);
      }
   }

   // Try to find module in library
   if (!success && (m_library != nullptr))
   {
      // Wildcard import
      if (importInfo->flags & MODULE_IMPORT_WILDCARD)
      {
         success = true;
         m_library->forEach(
            [vm, importInfo, &success] (const NXSL_LibraryScript *e) -> void
            {
               if (success && e->isValid() && MatchString(importInfo->name, e->getName(), false))
               {
                  NXSL_ModuleImport module;
                  _tcslcpy(module.name, e->getName(), MAX_IDENTIFIER_LENGTH);
                  module.lineNumber = importInfo->lineNumber;
                  module.flags = importInfo->flags & ~MODULE_IMPORT_WILDCARD;
                  success = vm->loadModule(e->getProgram(), &module);
               }
            });
      }
      else
      {
         NXSL_Program *libraryModule = m_library->findNxslProgram(importInfo->name);
         if (libraryModule != nullptr)
         {
            success = vm->loadModule(libraryModule, importInfo);
         }
      }
   }

   // If failed, try to load it from file
   if (!success && !(importInfo->flags & MODULE_IMPORT_WILDCARD))
   {
      TCHAR fileName[MAX_PATH];
      _sntprintf(fileName, MAX_PATH, _T("%s.nxsl"), importInfo->name);
      TCHAR *source = NXSLLoadFile(fileName);
      if (source != nullptr)
      {
         NXSL_CompilationDiagnostic diag;
         NXSL_Program *libraryModule = NXSLCompile(source, this, &diag);
         if (libraryModule != nullptr)
         {
            success = vm->loadModule(libraryModule, importInfo);
            delete libraryModule;
         }
         MemFree(source);
      }
   }

   return success || (importInfo->flags & MODULE_IMPORT_OPTIONAL);
}

/**
 * Write trace message
 * Default implementation does nothing
 */
void NXSL_Environment::trace(int level, const TCHAR *text)
{
}

/**
 * Print text to standard output
 * Default implementation writes value as string to stdout, if it is set
 */
void NXSL_Environment::print(const TCHAR *text)
{
   if (text != nullptr)
	{
      WriteToTerminal(text);
	}
   else
	{
      WriteToTerminal(_T("(null)"));
	}
}

/**
 * Additional VM configuration
 */
void NXSL_Environment::configureVM(NXSL_VM *vm)
{
}

/**
 * Get constant value known to this environment
 */
NXSL_Value *NXSL_Environment::getConstantValue(const NXSL_Identifier& name, NXSL_ValueManager *vm)
{
   if (name.value[0] == 'M')
   {
      NXSL_ENV_CONSTANT("MacAddressNotation::BYTEPAIR_COLON_SEPARATED", static_cast<int32_t>(MacAddressNotation::BYTEPAIR_COLON_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::BYTEPAIR_DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::BYTEPAIR_DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::COLON_SEPARATED", static_cast<int32_t>(MacAddressNotation::COLON_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::DECIMAL_DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::DECIMAL_DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::DOT_SEPARATED", static_cast<int32_t>(MacAddressNotation::DOT_SEPARATED));
      NXSL_ENV_CONSTANT("MacAddressNotation::FLAT_STRING", static_cast<int32_t>(MacAddressNotation::FLAT_STRING));
      NXSL_ENV_CONSTANT("MacAddressNotation::HYPHEN_SEPARATED", static_cast<int32_t>(MacAddressNotation::HYPHEN_SEPARATED));
   }

   if (name.value[0] == 'N')
   {
      NXSL_ENV_CONSTANT("NXSL::BuildTag", NETXMS_BUILD_TAG);
#if WORDS_BIGENDIAN
      NXSL_ENV_CONSTANT("NXSL::SystemIsBigEndian", true);
#else
      NXSL_ENV_CONSTANT("NXSL::SystemIsBigEndian", false);
#endif
      NXSL_ENV_CONSTANT("NXSL::Version", NETXMS_VERSION_STRING);
   }

   if (name.value[0] == 'S')
   {
      NXSL_ENV_CONSTANT("SeekOrigin::Begin", SEEK_SET);
      NXSL_ENV_CONSTANT("SeekOrigin::Current", SEEK_CUR);
      NXSL_ENV_CONSTANT("SeekOrigin::End", SEEK_END);
   }

   return nullptr;
}
