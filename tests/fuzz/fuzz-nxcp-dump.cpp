#include "fuzz-nxcp.h"

/**
 * Fuzz target for NXCPMessage::dump()
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   NXCP_MESSAGE *msg = CreateMessageFromFuzzerInput(data, size);
   if (msg == nullptr)
      return 0;

   int version = ntohs(msg->flags) >> 12;
   NXCPMessage::dump(msg, (version != 0) ? version : NXCP_VERSION);
   MemFree(msg);
   return 0;
}
