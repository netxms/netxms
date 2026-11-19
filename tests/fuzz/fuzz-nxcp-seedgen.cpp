#include "fuzz-nxcp.h"

static const char *s_corpusDirectory;

/**
 * Write seed file into given subdirectory of corpus directory
 */
static void WriteSeed(const char *target, const char *name, const void *data, size_t size)
{
   char path[MAX_PATH];
   snprintf(path, MAX_PATH, "%s/%s/seed-%s", s_corpusDirectory, target, name);
   FILE *f = fopen(path, "wb");
   if (f == nullptr)
   {
      fprintf(stderr, "Cannot create %s\n", path);
      exit(1);
   }
   fwrite(data, 1, size, f);
   fclose(f);
}

/**
 * Write seed file for message targets (deserialize and dump)
 */
static void WriteSeed(const char *name, const void *data, size_t size)
{
   WriteSeed("message", name, data, size);
}

/**
 * Write serialized form of given message as seed file for message targets
 */
static void WriteSeed(const char *name, const NXCPMessage& msg, bool compress = false)
{
   NXCP_MESSAGE *data = msg.serialize(compress);
   if (compress && !(ntohs(data->flags) & MF_COMPRESSED))
   {
      fprintf(stderr, "Message for seed %s was not compressed\n", name);
      exit(1);
   }
   WriteSeed(name, data, ntohl(data->size));
   MemFree(data);
}

/**
 * Field kinds written into seed messages
 */
static const char *s_fieldNames[] =
{
   "int16", "int32", "int64", "float", "string", "emptystring", "binary", "inetaddr4", "inetaddr6", "guid", "mac", "int32array", "json"
};
static const uint32_t s_fieldCount = sizeof(s_fieldNames) / sizeof(s_fieldNames[0]);

/**
 * Set field of given kind (index in s_fieldNames); field ID is index + 1
 */
static void SetField(NXCPMessage *msg, uint32_t index)
{
   BYTE binary[40];
   memset(binary, 0xA5, sizeof(binary));
   uint32_t integers[] = { 1, 2, 3, 0xFFFFFFFF };

   uint32_t fieldId = index + 1;
   switch(index)
   {
      case 0:
         msg->setField(fieldId, static_cast<int16_t>(-2));
         break;
      case 1:
         msg->setField(fieldId, static_cast<uint32_t>(0x80000001));
         break;
      case 2:
         msg->setField(fieldId, static_cast<uint64_t>(_ULL(0x8000000000000001)));
         break;
      case 3:
         msg->setField(fieldId, 3.14159);
         break;
      case 4:
         msg->setField(fieldId, _T("Text with non-ASCII characters: \x444\x44B\x432\x430"));
         break;
      case 5:
         msg->setField(fieldId, _T(""));
         break;
      case 6:
         msg->setField(fieldId, binary, sizeof(binary));
         break;
      case 7:
         msg->setField(fieldId, InetAddress::parse("192.168.10.20"));
         break;
      case 8:
         msg->setField(fieldId, InetAddress::parse("2001:db8::1:0:0:1"));
         break;
      case 9:
         msg->setField(fieldId, uuid::parse(_T("3b1a8e5c-7d0f-4c61-9a52-0e6f1b2c3d4e")));
         break;
      case 10:
         msg->setField(fieldId, MacAddress::parse("00:11:22:33:44:55"));
         break;
      case 11:
         msg->setFieldFromInt32Array(fieldId, sizeof(integers) / sizeof(uint32_t), integers);
         break;
      case 12:
         msg->setFieldFromUtf8String(fieldId, "{ \"name\": \"value\", \"list\": [ 1, 2, 3 ] }");
         break;
   }
}

/**
 * Write one seed per field kind and one with all of them for given protocol version
 */
static void WriteFieldSeeds(int version)
{
   char name[64];
   NXCPMessage all(CMD_REQUEST_COMPLETED, 100, version);
   for(uint32_t i = 0; i < s_fieldCount; i++)
   {
      SetField(&all, i);

      NXCPMessage single(CMD_REQUEST_COMPLETED, i + 1, version);
      SetField(&single, i);
      snprintf(name, sizeof(name), "v%d-%s", version, s_fieldNames[i]);
      WriteSeed(name, single);
   }
   snprintf(name, sizeof(name), "v%d-all-fields", version);
   WriteSeed(name, all);
}

/**
 * Byte stream built from messages
 */
class Stream
{
private:
   ByteStream m_data;

public:
   Stream() : m_data(4096) { }

   void add(const void *data, size_t size) { m_data.write(data, size); }
   void add(const NXCP_MESSAGE *msg) { add(msg, ntohl(msg->size)); }
   void add(const NXCPMessage& msg)
   {
      NXCP_MESSAGE *data = msg.serialize();
      add(data);
      MemFree(data);
   }

   void write(const char *target, const char *name) { WriteSeed(target, name, m_data.buffer(), m_data.size()); }
};

/**
 * Create message with one binary field of given size
 */
static NXCPMessage CreateMessage(uint32_t id, size_t binaryFieldSize)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, id);
   if (binaryFieldSize > 0)
   {
      BYTE *data = MemAllocArrayNoInit<BYTE>(binaryFieldSize);
      memset(data, 0x30 + id, binaryFieldSize);
      msg.setField(1, data, binaryFieldSize);
      MemFree(data);
   }
   return msg;
}

/**
 * Write seeds for receiver target: streams of clear text messages. Receiver in that target starts with
 * 64 byte buffer and does not accept messages larger than 1024 bytes.
 */
static void WriteReceiverSeeds()
{
   Stream small;
   small.add(CreateMessage(1, 0));
   small.add(CreateMessage(2, 8));
   small.add(CreateMessage(3, 16));
   small.write("receiver", "small-messages");

   Stream grow;
   grow.add(CreateMessage(1, 0));
   grow.add(CreateMessage(2, 200));
   grow.add(CreateMessage(3, 600));
   grow.add(CreateMessage(4, 8));
   grow.write("receiver", "buffer-growth");

   Stream skip;
   skip.add(CreateMessage(1, 8));
   skip.add(CreateMessage(2, 2000));
   skip.add(CreateMessage(3, 8));
   skip.write("receiver", "oversized-message-skipped");

   Stream garbage;
   garbage.add("\x00\x01\x02\x03\x00\x00\x00\x05garbage!", 16);
   garbage.add(CreateMessage(1, 8));
   garbage.write("receiver", "garbage-before-message");

   NXCPMessage compressible(CMD_REQUEST_COMPLETED, 2);
   BYTE zeroes[800];
   memset(zeroes, 0, sizeof(zeroes));
   compressible.setField(1, zeroes, sizeof(zeroes));
   NXCP_MESSAGE *compressed = compressible.serialize(true);
   Stream mixed;
   mixed.add(CreateMessage(1, 8));
   mixed.add(compressed);
   mixed.add(CreateMessage(3, 8));
   mixed.write("receiver", "compressed-message");
   MemFree(compressed);
}

/**
 * Write seeds for decrypt target. First byte selects the mode: 0 for stream of envelopes, 1 for clear text
 * frame that the target encrypts itself.
 */
static void WriteDecryptSeeds()
{
   shared_ptr<NXCPEncryptionContext> context = CreateFuzzEncryptionContext();
   static const size_t sizes[] = { 0, 8, 200, 600 };

   Stream envelopes;
   envelopes.add("\x00", 1);
   for(uint32_t i = 0; i < 4; i++)
   {
      NXCP_MESSAGE *clear = CreateMessage(i + 1, sizes[i]).serialize();
      NXCP_ENCRYPTED_MESSAGE *encrypted = context->encryptMessage(clear);
      envelopes.add(encrypted, ntohl(encrypted->size));

      Stream envelope;
      envelope.add("\x00", 1);
      envelope.add(encrypted, ntohl(encrypted->size));
      Stream frame;
      frame.add("\x01", 1);
      frame.add(clear);

      char name[64];
      snprintf(name, sizeof(name), "envelope-%u", static_cast<unsigned int>(sizes[i]));
      envelope.write("decrypt", name);
      snprintf(name, sizeof(name), "frame-%u", static_cast<unsigned int>(sizes[i]));
      frame.write("decrypt", name);

      MemFree(encrypted);
      MemFree(clear);
   }
   envelopes.write("decrypt", "envelope-stream");

   Stream mixed;
   mixed.add("\x00", 1);
   NXCP_MESSAGE *clear = CreateMessage(1, 8).serialize();
   NXCP_ENCRYPTED_MESSAGE *encrypted = context->encryptMessage(clear);
   mixed.add(clear);
   mixed.add(encrypted, ntohl(encrypted->size));
   mixed.add(clear);
   mixed.write("decrypt", "clear-and-encrypted");
   MemFree(encrypted);
   MemFree(clear);
}

/**
 * Seed generator for NXCP fuzz targets. Writes valid input for each target into subdirectories
 * message, receiver and decrypt of given corpus directory.
 */
int main(int argc, char *argv[])
{
   if (argc != 2)
   {
      fprintf(stderr, "Usage: fuzz-nxcp-seedgen <corpus directory>\n");
      return 1;
   }
   s_corpusDirectory = argv[1];

   WriteFieldSeeds(4);
   WriteFieldSeeds(NXCP_VERSION);

   NXCPMessage empty(CMD_KEEPALIVE, 1);
   WriteSeed("empty", empty);

   NXCP_MESSAGE control;
   memset(&control, 0, sizeof(control));
   control.code = htons(CMD_GET_NXCP_CAPS);
   control.flags = htons(MF_CONTROL);
   control.size = htonl(NXCP_HEADER_SIZE);
   control.numFields = htonl(NXCP_VERSION << 24);
   WriteSeed("control", &control, NXCP_HEADER_SIZE);

   BYTE text[600];
   for(size_t i = 0; i < sizeof(text); i++)
      text[i] = static_cast<BYTE>('a' + i % 7);

   NXCP_MESSAGE *binary = CreateRawNXCPMessage(CMD_FILE_DATA, 1, 0, text, 100, nullptr, false);
   WriteSeed("binary", binary, ntohl(binary->size));
   MemFree(binary);

   binary = CreateRawNXCPMessage(CMD_FILE_DATA, 2, 0, text, sizeof(text), nullptr, true);
   if (!(ntohs(binary->flags) & MF_COMPRESSED))
   {
      fprintf(stderr, "Binary message was not compressed\n");
      return 1;
   }
   WriteSeed("binary-compressed", binary, ntohl(binary->size));
   MemFree(binary);

   binary = CreateRawNXCPMessage(CMD_FILE_DATA, 3, MF_STREAM | MF_END_OF_FILE, text, 100, nullptr, false);
   WriteSeed("binary-stream", binary, ntohl(binary->size));
   MemFree(binary);

   NXCPMessage compressed(CMD_REQUEST_COMPLETED, 200);
   for(uint32_t i = 0; i < s_fieldCount; i++)
      SetField(&compressed, i);
   compressed.setField(s_fieldCount + 1, text, sizeof(text));
   WriteSeed("compressed", compressed, true);

   NXCPMessage repeated(CMD_REQUEST_COMPLETED, 201);
   for(uint32_t fieldId = 1; fieldId <= 200; fieldId++)
      repeated.setField(fieldId, static_cast<uint32_t>(42));
   WriteSeed("compressed-many-fields", repeated, true);

   WriteReceiverSeeds();
   WriteDecryptSeeds();

   return 0;
}
