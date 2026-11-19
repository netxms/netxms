#include "fuzz-nxcp.h"
#include <set>

/**
 * Value that compiler cannot discard, used to force reads of data returned by getters
 */
static volatile uint32_t s_sink;

/**
 * Read every byte of given memory block
 */
static void Touch(const void *data, size_t size)
{
   uint32_t sum = 0;
   for(size_t i = 0; i < size; i++)
      sum += static_cast<const BYTE*>(data)[i];
   s_sink = sum;
}

/**
 * Call every getter for given field
 */
static void ReadField(const NXCPMessage& msg, uint32_t fieldId)
{
   msg.getFieldType(fieldId);
   msg.getFieldAsInt16(fieldId);
   msg.getFieldAsUInt16(fieldId);
   msg.getFieldAsInt32(fieldId);
   msg.getFieldAsUInt32(fieldId);
   msg.getFieldAsInt64(fieldId);
   msg.getFieldAsUInt64(fieldId);
   msg.getFieldAsDouble(fieldId);
   msg.getFieldAsBoolean(fieldId);
   msg.getFieldAsTime(fieldId);
   msg.getFieldAsTimestamp(fieldId);

   uint32_t elements[4];
   msg.getFieldAsInt32Array(fieldId, 4, elements);
   IntegerArray<uint32_t> array;
   msg.getFieldAsInt32Array(fieldId, &array);

   size_t size;
   const BYTE *data = msg.getBinaryFieldPtr(fieldId, &size);
   if (data != nullptr)
      Touch(data, size);
   BYTE binaryBuffer[8];
   msg.getFieldAsBinary(fieldId, binaryBuffer, sizeof(binaryBuffer));

   TCHAR smallBuffer[4];
   msg.getFieldAsString(fieldId, smallBuffer, 4);
   MemFree(msg.getFieldAsString(fieldId));
   MemoryPool pool;
   msg.getFieldAsString(fieldId, &pool);
   Buffer<TCHAR, 64> buffer;
   msg.getFieldAsString(fieldId, buffer);
   msg.getFieldAsSharedString(fieldId);
   msg.getFieldAsSharedString(fieldId, 4);
   msg.getFieldAsString(fieldId, static_cast<size_t>(4));

   char smallBufferA[4];
   msg.getFieldAsMBString(fieldId, smallBufferA, 4);
   MemFree(msg.getFieldAsMBString(fieldId));
   msg.getFieldAsUtf8String(fieldId, smallBufferA, 4);
   MemFree(msg.getFieldAsUtf8String(fieldId));
   Buffer<char, 64> bufferA;
   msg.getFieldAsUtf8String(fieldId, bufferA);

   msg.getFieldAsInetAddress(fieldId);
   msg.getFieldAsMacAddress(fieldId);
   msg.getFieldAsGUID(fieldId);
   json_decref(msg.getFieldAsJson(fieldId));
}

/**
 * Fuzz target for NXCPMessage::deserialize() and field getters
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   NXCP_MESSAGE *input = CreateMessageFromFuzzerInput(data, size);
   if (input == nullptr)
      return 0;

   // Protocol versions 1 to 3 are not supported, and serialize() of such message is known to access unaligned string
   int version = ntohs(input->flags) >> 12;
   if ((version != 0) && (version < 4))
   {
      MemFree(input);
      return 0;
   }

   NXCPMessage *msg = NXCPMessage::deserialize(input);
   MemFree(input);
   if (msg == nullptr)
      return 0;

   NXCP_MESSAGE *serialized = msg->serialize(false);
   if (msg->isBinary())
   {
      Touch(msg->getBinaryData(), msg->getBinaryDataSize());
   }
   else if (!msg->isControl())
   {
      // NXCPMessage has no field enumerator. Every field ID is present in serialized message, so each
      // 32 bit value in it is tried as an ID.
      std::set<uint32_t> fields;
      size_t serializedSize = ntohl(serialized->size);
      for(size_t offset = NXCP_HEADER_SIZE; offset + 4 <= serializedSize; offset++)
      {
         uint32_t fieldId;
         memcpy(&fieldId, reinterpret_cast<const BYTE*>(serialized) + offset, 4);
         fieldId = ntohl(fieldId);
         if (msg->isFieldExist(fieldId))
            fields.insert(fieldId);
      }
      for(uint32_t fieldId : fields)
         ReadField(*msg, fieldId);
   }
   MemFree(serialized);
   MemFree(msg->serialize(true));

   delete msg;
   return 0;
}
