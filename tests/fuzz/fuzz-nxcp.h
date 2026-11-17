#ifndef _fuzz_nxcp_h_
#define _fuzz_nxcp_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>

/**
 * Create message that meets the guarantees given by message receiver to NXCPMessage::deserialize() and
 * NXCPMessage::dump(): at least one header long, aligned to 8 bytes, with size field equal to number of
 * bytes available. Message is placed into heap block of exact size, so that any read past its end is
 * reported by address sanitizer. Returns nullptr if input is too short.
 */
static inline NXCP_MESSAGE *CreateMessageFromFuzzerInput(const uint8_t *data, size_t size)
{
   size -= size % 8;
   if (size < NXCP_HEADER_SIZE)
      return nullptr;

   NXCP_MESSAGE *msg = static_cast<NXCP_MESSAGE*>(MemAlloc(size));
   memcpy(msg, data, size);
   msg->size = htonl(static_cast<uint32_t>(size));
   return msg;
}

/**
 * Message receiver that reads fuzzer input. Input is delivered unchanged; size of each read is derived from
 * the byte at current position, so that the input controls how messages are split between reads.
 */
class FuzzMessageReceiver : public AbstractMessageReceiver
{
private:
   const uint8_t *m_input;
   size_t m_inputSize;
   size_t m_position;

protected:
   virtual ssize_t readBytes(BYTE *buffer, size_t size, uint32_t timeout) override
   {
      if (m_position == m_inputSize)
         return 0;
      uint8_t selector = m_input[m_position];
      size_t bytes = std::min(m_inputSize - m_position, size);
      if ((selector & 0x03) != 0)   // one read out of four takes everything that fits
         bytes = std::min(bytes, static_cast<size_t>(selector >> 2) + 1);
      memcpy(buffer, &m_input[m_position], bytes);
      m_position += bytes;
      return bytes;
   }

public:
   FuzzMessageReceiver(const uint8_t *input, size_t inputSize, size_t initialSize, size_t maxSize) : AbstractMessageReceiver(initialSize, maxSize)
   {
      m_input = input;
      m_inputSize = inputSize;
      m_position = 0;
   }

   virtual void cancel() override { }
};

/**
 * Read messages until receiver reports anything but success and dump each one from raw receive buffer,
 * the way server and agent sessions do at debug level 8. Decryption failure does not stop reading
 * when requested (agent and mobile device sessions continue after it).
 */
static inline void ReadMessages(AbstractMessageReceiver *receiver, bool continueAfterDecryptionFailure)
{
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver->readMessage(0, &result);
      if (result == MSGRECV_SUCCESS)
      {
         NXCPMessage::dump(receiver->getRawMessageBuffer(), NXCP_VERSION);
         delete msg;
      }
      else if ((result != MSGRECV_DECRYPTION_FAILURE) || !continueAfterDecryptionFailure)
      {
         break;
      }
   }
}

/**
 * Create AES-256 encryption context with fixed key and IV, so that encrypted corpus entries stay valid
 */
static inline shared_ptr<NXCPEncryptionContext> CreateFuzzEncryptionContext()
{
   shared_ptr<NXCPEncryptionContext> context(NXCPEncryptionContext::create(NXCP_SUPPORT_AES_256));
   if (context == nullptr)
   {
      fprintf(stderr, "Cannot create AES-256 encryption context\n");
      abort();
   }
   memset(context->getSessionKey(), 0x4B, context->getKeyLength());
   memset(context->getIV(), 0x56, EVP_MAX_IV_LENGTH);
   return context;
}

/**
 * Largest clear text frame that inner frame mode of decrypt target will encrypt
 */
#define FUZZ_MAX_INNER_FRAME_SIZE   4096

/**
 * Encrypt frame given as fuzzer input. Size declared in frame header is kept as is, because the receiver has to
 * cope with any value there, but NXCPEncryptionContext::encryptMessage() reads that many bytes, so the frame is
 * copied into zero padded buffer of at least declared size. Returns nullptr if declared size is above the limit.
 */
static inline NXCP_ENCRYPTED_MESSAGE *EncryptFuzzerFrame(NXCPEncryptionContext *context, const uint8_t *data, size_t size)
{
   BYTE header[NXCP_HEADER_SIZE];
   memset(header, 0, NXCP_HEADER_SIZE);
   memcpy(header, data, std::min(size, static_cast<size_t>(NXCP_HEADER_SIZE)));
   uint32_t declaredSize;
   memcpy(&declaredSize, &header[4], 4);
   declaredSize = ntohl(declaredSize);
   if (declaredSize > FUZZ_MAX_INNER_FRAME_SIZE)
      return nullptr;

   size_t frameSize = std::max(static_cast<size_t>(declaredSize), static_cast<size_t>(NXCP_HEADER_SIZE));
   BYTE *frame = MemAllocArray<BYTE>(frameSize);
   memcpy(frame, data, std::min(size, frameSize));
   NXCP_ENCRYPTED_MESSAGE *encrypted = context->encryptMessage(reinterpret_cast<NXCP_MESSAGE*>(frame));
   MemFree(frame);
   return encrypted;
}

#endif
