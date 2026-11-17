#include "fuzz-nxcp.h"

/**
 * Fuzz target for encrypted message processing. First byte of input selects the mode:
 *    even - the rest is received as is by receiver with encryption context (encrypted message envelopes);
 *    odd  - the rest is one clear text frame, which is encrypted and then received, so that content of
 *           decrypted message is under fuzzer control despite the checksum.
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   static shared_ptr<NXCPEncryptionContext> context = CreateFuzzEncryptionContext();

   if (size < 1)
      return 0;

   // Size limit is above the largest envelope produced in inner frame mode
   if (data[0] & 1)
   {
      NXCP_ENCRYPTED_MESSAGE *encrypted = EncryptFuzzerFrame(context.get(), data + 1, size - 1);
      if (encrypted == nullptr)
         return 0;
      FuzzMessageReceiver receiver(reinterpret_cast<uint8_t*>(encrypted), ntohl(encrypted->size), 64, FUZZ_MAX_INNER_FRAME_SIZE * 2);
      receiver.setEncryptionContext(context);
      ReadMessages(&receiver, true);
      MemFree(encrypted);
   }
   else
   {
      FuzzMessageReceiver receiver(data + 1, size - 1, 64, FUZZ_MAX_INNER_FRAME_SIZE * 2);
      receiver.setEncryptionContext(context);
      ReadMessages(&receiver, true);
   }
   return 0;
}
