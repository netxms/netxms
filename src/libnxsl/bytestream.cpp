/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2022 Raden Solutions
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
** File: bytestream.cpp
**
**/

#include "libnxsl.h"

/**
 * Instance of NXSL_ByteStream class
 */
NXSL_ByteStreamClass LIBNXSL_EXPORTABLE g_nxslByteStreamClass;

/**
 * ByteStream::seek(position, [whence]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, seek)
{
   if (argc < 1 || argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isInteger() || (argc > 1 && !argv[1]->isInteger()))
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   *result = vm->createValue(static_cast<int64_t>(bs->seek(argv[0]->getValueAsInt32(), (argc > 1) ? argv[1]->getValueAsInt32() : SEEK_SET)));
   return 0;
}

/**
 * Read from byte stream using method passed as argument
 */
#define READ_FROM_BYTE_STREAM(method)                            \
   ByteStream *bs = static_cast<ByteStream*>(object->getData()); \
   *result = vm->createValue(bs->method());                      \
   return 0;

/**
 * ByteStream::readByte() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readByte)
{
   READ_FROM_BYTE_STREAM(readByte);
}

/**
 * ByteStream::readInt16B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt16B)
{
   READ_FROM_BYTE_STREAM(readInt16B);
}

/**
 * ByteStream::readInt32B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt32B)
{
   READ_FROM_BYTE_STREAM(readInt32B);
}

/**
 * ByteStream::readInt64B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt64B)
{
   READ_FROM_BYTE_STREAM(readInt64B);
}

/**
 * ByteStream::readUInt16B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt16B)
{
   READ_FROM_BYTE_STREAM(readUInt16B);
}

/**
 * ByteStream::readUInt32B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt32B)
{
   READ_FROM_BYTE_STREAM(readUInt32B);
}

/**
 * ByteStream::readUInt64B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt64B)
{
   READ_FROM_BYTE_STREAM(readUInt64B);
}

/**
 * ByteStream::readFloat32B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readFloat32B)
{
   READ_FROM_BYTE_STREAM(readFloatB);
}

/**
 * ByteStream::readFloat64B() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readFloat64B)
{
   READ_FROM_BYTE_STREAM(readDoubleB);
}

/**
 * ByteStream::readInt16L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt16L)
{
   READ_FROM_BYTE_STREAM(readInt16L);
}

/**
 * ByteStream::readInt32L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt32L)
{
   READ_FROM_BYTE_STREAM(readInt32L);
}

/**
 * ByteStream::readInt64L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readInt64L)
{
   READ_FROM_BYTE_STREAM(readInt64L);
}

/**
 * ByteStream::readUInt16L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt16L)
{
   READ_FROM_BYTE_STREAM(readUInt16L);
}

/**
 * ByteStream::readUInt32L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt32L)
{
   READ_FROM_BYTE_STREAM(readUInt32L);
}

/**
 * ByteStream::readU Int64L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readUInt64L)
{
   READ_FROM_BYTE_STREAM(readUInt64L);
}

/**
 * ByteStream::readFloat32L() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readFloat32L)
{
   READ_FROM_BYTE_STREAM(readFloatL);
}

/**
 * ByteStream::readFloatL() method
 */
NXSL_METHOD_DEFINITION(ByteStream, readFloat64L)
{
   READ_FROM_BYTE_STREAM(readDoubleL);
}

/**
 * ByteStream::readString(length, [codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, readString)
{
   if (argc < 1 || argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (argc > 0 && !argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   if (argc > 1 && !argv[1]->isString())
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   const char* codepage = (argc > 1) ? argv[1]->getValueAsMBString() : nullptr;
   WCHAR* buffer = bs->readStringW(codepage, argv[0]->getValueAsInt32());
   *result = (buffer != nullptr) ? vm->createValue(buffer) : vm->createValue();
   MemFree(buffer);
   return 0;
}

/**
 * ByteStream::readCString([codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, readCString)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (argc > 0 && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   const char* codepage = argc > 0 ? argv[0]->getValueAsMBString() : nullptr;
   WCHAR* buffer = bs->readCStringW(codepage);
   *result = (buffer != nullptr) ? vm->createValue(buffer) : vm->createValue();
   MemFree(buffer);
   return 0;
}

/**
 * ByteStream::readLengthPrependedString([codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, readPString)
{
   if (argc > 1)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (argc > 0 && !argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   const char* codepage = argc > 0 ? argv[0]->getValueAsMBString() : nullptr;
   WCHAR* buffer = bs->readPStringW(codepage);
   *result = (buffer != nullptr) ? vm->createValue(buffer) : nullptr;
   MemFree(buffer);
   return 0;
}

/**
 * ByteStream::writeByte(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeByte)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->write(static_cast<BYTE>(argv[0]->getValueAsInt32()));
   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt16B(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt16B)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeB(static_cast<int16_t>(argv[0]->getValueAsInt32()));

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt32B(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt32B)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeB(argv[0]->getValueAsInt32());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt64B(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt64B)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeB(argv[0]->getValueAsInt64());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeFloat64B(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeFloat64B)
{
   if (!argv[0]->isReal())
      return NXSL_ERR_NOT_NUMBER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeB(argv[0]->getValueAsReal());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeFloat32B(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeFloat32B)
{
   if (!argv[0]->isReal())
      return NXSL_ERR_NOT_NUMBER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeB(static_cast<float>(argv[0]->getValueAsReal()));

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt16L(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt16L)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeL(static_cast<int16_t>(argv[0]->getValueAsInt32()));

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt32L(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt32L)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeL(argv[0]->getValueAsInt32());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeInt64L(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeInt64L)
{
   if (!argv[0]->isInteger())
      return NXSL_ERR_NOT_INTEGER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeL(argv[0]->getValueAsInt64());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeFloat32L(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeFloat32L)
{
   if (!argv[0]->isReal())
      return NXSL_ERR_NOT_NUMBER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeL(static_cast<float>(argv[0]->getValueAsReal()));

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeFloat64L(value) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeFloat64L)
{
   if (!argv[0]->isReal())
      return NXSL_ERR_NOT_NUMBER;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   bs->writeL(argv[0]->getValueAsReal());

   *result = vm->createValue();
   return 0;
}

/**
 * ByteStream::writeString(string, [codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeString)
{
   if (argc < 1 || argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isString() || (argc > 1 && !argv[1]->isString() && !argv[1]->isNull()))
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   uint32_t len;
   const WCHAR* str = argv[0]->getValueAsString(&len);
   ssize_t count = bs->writeString(
      str,
      argc > 1 ? argv[1]->getValueAsMBString() : nullptr,
      len, false, false);

   *result = vm->createValue(static_cast<int64_t>(count));
   return 0;
}

/**
 * ByteStream::writeCString(string, [codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writeCString)
{
   if (argc < 1 || argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;
   if (!argv[0]->isString() || (argc > 1 && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   uint32_t len;
   const WCHAR* str = argv[0]->getValueAsString(&len);
   ssize_t count = bs->writeString(str, (argc > 1) ? argv[1]->getValueAsMBString() : nullptr, len, false, true);

   *result = vm->createValue(static_cast<int64_t>(count));
   return 0;
}

/**
 * ByteStream::writePString(string, [codepage]) method
 */
NXSL_METHOD_DEFINITION(ByteStream, writePString)
{
   if (argc < 1 || argc > 2)
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || (argc > 1 && !argv[1]->isString()))
      return NXSL_ERR_NOT_STRING;

   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   uint32_t len;
   const WCHAR* str = argv[0]->getValueAsString(&len);
   ssize_t count = bs->writeString(str, (argc > 1) ? argv[1]->getValueAsMBString() : nullptr, len, true, false);

   *result = vm->createValue(static_cast<int64_t>(count));
   return 0;
}

/**
 * Implementation of "ByteStream" class: constructor
 */
NXSL_ByteStreamClass::NXSL_ByteStreamClass()
   : NXSL_Class()
{
   setName(_T("ByteStream"));

   NXSL_REGISTER_METHOD(ByteStream, seek, -1);

   // Read
   NXSL_REGISTER_METHOD(ByteStream, readByte, 0);

   NXSL_REGISTER_METHOD(ByteStream, readInt16B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readInt32B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readInt64B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt16B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt32B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt64B, 0);
   NXSL_REGISTER_METHOD_ALIAS(ByteStream, readFloatB, 0, readFloat64B);
   NXSL_REGISTER_METHOD(ByteStream, readFloat32B, 0);
   NXSL_REGISTER_METHOD(ByteStream, readFloat64B, 0);

   NXSL_REGISTER_METHOD(ByteStream, readInt16L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readInt32L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readInt64L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt16L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt32L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readUInt64L, 0);
   NXSL_REGISTER_METHOD_ALIAS(ByteStream, readFloatL, 0, readFloat64L);
   NXSL_REGISTER_METHOD(ByteStream, readFloat32L, 0);
   NXSL_REGISTER_METHOD(ByteStream, readFloat64L, 0);

   NXSL_REGISTER_METHOD(ByteStream, readCString, -1);
   NXSL_REGISTER_METHOD(ByteStream, readPString, -1);
   NXSL_REGISTER_METHOD(ByteStream, readString, -1);

   // Write
   NXSL_REGISTER_METHOD(ByteStream, writeByte, 1);

   NXSL_REGISTER_METHOD(ByteStream, writeInt16B, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeInt32B, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeInt64B, 1);
   NXSL_REGISTER_METHOD_ALIAS(ByteStream, writeFloatB, 1, writeFloat64B);
   NXSL_REGISTER_METHOD(ByteStream, writeFloat32B, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeFloat64B, 1);

   NXSL_REGISTER_METHOD(ByteStream, writeInt16L, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeInt32L, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeInt64L, 1);
   NXSL_REGISTER_METHOD_ALIAS(ByteStream, writeFloatL, 1, writeFloat64L);
   NXSL_REGISTER_METHOD(ByteStream, writeFloat32L, 1);
   NXSL_REGISTER_METHOD(ByteStream, writeFloat64L, 1);

   NXSL_REGISTER_METHOD(ByteStream, writeCString, -1);
   NXSL_REGISTER_METHOD(ByteStream, writePString, -1);
   NXSL_REGISTER_METHOD(ByteStream, writeString, -1);
}

/**
 * Implementation of "ByteStreamClass" class: get attribute
 */
NXSL_Value* NXSL_ByteStreamClass::getAttr(NXSL_Object* object, const NXSL_Identifier& attr)
{
   NXSL_Value* value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM* vm = object->vm();
   ByteStream* bs = static_cast<ByteStream*>(object->getData());
   if (NXSL_COMPARE_ATTRIBUTE_NAME("size"))
   {
      value = vm->createValue(static_cast<int64_t>(bs->size()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("pos"))
   {
      value = vm->createValue(static_cast<int64_t>(bs->pos()));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("eos"))
   {
      value = vm->createValue(bs->eos());
   }
   return value;
}

/**
 * Object delete
 */
void NXSL_ByteStreamClass::onObjectDelete(NXSL_Object* object)
{
   delete static_cast<ByteStream*>(object->getData());
}

/**
 * NXSL constructor for "ByteStream" class
 */
int F_ByteStream(int argc, NXSL_Value** argv, NXSL_Value** result, NXSL_VM* vm)
{
   *result = vm->createValue(vm->createObject(&g_nxslByteStreamClass, new ByteStream()));
   return 0;
}
