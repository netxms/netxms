#include "fuzz-nxcp.h"

/**
 * Fuzz target for message receiver (framing of clear text messages). Receive buffer starts small and has
 * low size limit, so that buffer growth and skipping of oversized messages are reachable with short inputs.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   FuzzMessageReceiver receiver(data, size, 64, 1024);
   ReadMessages(&receiver, false);
   return 0;
}
