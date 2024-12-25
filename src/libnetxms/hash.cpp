/* 
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: hash.cpp
**
**/

#include "libnetxms.h"
#include <nxcrypto.h>
#include "md5.h"
#include "sha1.h"
#include "sha2.h"

#ifdef _WITH_ENCRYPTION
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#endif

#ifdef _WIN32
#include <io.h>
#endif

#include <nxcrypto.h>

/**
 * File block size (used for file hash calculation)
 */
#define FILE_BLOCK_SIZE    4096

/**
 * Table for CRC calculation
 */
static uint32_t crctab[256]=
{
	0x00000000, 
	0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 
	0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 
	0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 
	0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 
	0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 
	0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 
	0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 
	0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 
	0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 
	0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 
	0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 
	0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106, 
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 
	0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 
	0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 
	0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 
	0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 
	0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 
	0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 
	0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 
	0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 
	0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 
	0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 
	0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 
	0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 
	0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344, 0x8708a3d2, 
	0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 
	0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 
	0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 
	0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 
	0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 
	0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 
	0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 
	0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 
	0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 
	0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 
	0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 
	0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 
	0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 
	0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 
	0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 
	0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 
	0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 
	0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/**
 * Calculate CRC32 for buffer of specified length
 */
uint32_t LIBNETXMS_EXPORTABLE CalculateCRC32(const BYTE *data, size_t size, uint32_t crc)
{
	crc = ~crc;
	while (size-- != 0)
	{
		crc = (crc >> 8) ^ crctab[(crc & 0xFF) ^ *data++];
	}
	return ~crc;
}

/**
 * Calculate CRC32 for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileCRC32(const TCHAR *fileName, uint32_t *result)
{
   bool success = false;

   FILE *fileHandle = _tfopen(fileName, _T("rb"));
   if (fileHandle != nullptr)
   {
      BYTE buffer[FILE_BLOCK_SIZE];
      *result = 0;
      while(true)
      {
         size_t size = fread(buffer, 1, FILE_BLOCK_SIZE, fileHandle);
         if (size == 0)
         {
            success = true;
            break;
         }
         *result = CalculateCRC32(buffer, size, *result);
      }
      fclose(fileHandle);
   }
   return success;
}

/**
 * Init MD5 hash function
 */
void LIBNETXMS_EXPORTABLE MD5Init(MD5_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_md5(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_md5(), nullptr);
#endif
#else
   I_md5_init(reinterpret_cast<md5_state_t*>(state));
#endif
}

/**
 * Append data to MD5 hash
 */
void LIBNETXMS_EXPORTABLE MD5Update(MD5_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_md5_append(reinterpret_cast<md5_state_t*>(state), static_cast<const md5_byte_t*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish MD5 calculation
 */
void LIBNETXMS_EXPORTABLE MD5Final(MD5_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_md5_finish(reinterpret_cast<md5_state_t*>(state), reinterpret_cast<md5_byte_t*>(hash));
#endif
}

/**
 * Calculate MD5 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateMD5Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
   EVP_MD_CTX *context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(context, EVP_md5(), nullptr);
   EVP_DigestUpdate(context, data, size);
   EVP_DigestFinal_ex(context, hash, nullptr);
   EVP_MD_CTX_free(context);
#else
   MD5(static_cast<const unsigned char*>(data), size, hash);
#endif
#else
   md5_state_t state;
   I_md5_init(&state);
   I_md5_append(&state, (const md5_byte_t *)data, static_cast<unsigned int>(size));
   I_md5_finish(&state, (md5_byte_t *)hash);
#endif
}

/**
 * Init SHA1 hash function
 */
void LIBNETXMS_EXPORTABLE SHA1Init(SHA1_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_sha1(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_sha1(), nullptr);
#endif
#else
   I_SHA1Init(reinterpret_cast<SHA1_CTX*>(state));
#endif
}

/**
 * Append data to SHA1 hash
 */
void LIBNETXMS_EXPORTABLE SHA1Update(SHA1_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_SHA1Update(reinterpret_cast<SHA1_CTX*>(state), static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish SHA1 calculation
 */
void LIBNETXMS_EXPORTABLE SHA1Final(SHA1_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_SHA1Final(reinterpret_cast<SHA1_CTX*>(state), hash);
#endif
}

/**
 * Calculate SHA1 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateSHA1Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
   SHA1(static_cast<const unsigned char*>(data), size, hash);
#else
   SHA1_CTX context;
   I_SHA1Init(&context);
   I_SHA1Update(&context, static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
   I_SHA1Final(&context, hash);
#endif
}

/**
 * Init SHA224 hash function
 */
void LIBNETXMS_EXPORTABLE SHA224Init(SHA224_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_sha224(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_sha224(), nullptr);
#endif
#else
   I_sha224_init(reinterpret_cast<sha224_ctx*>(state));
#endif
}

/**
 * Append data to SHA224 hash
 */
void LIBNETXMS_EXPORTABLE SHA224Update(SHA224_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_sha224_update(reinterpret_cast<sha224_ctx*>(state), static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish SHA256 calculation
 */
void LIBNETXMS_EXPORTABLE SHA224Final(SHA224_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_sha224_final(reinterpret_cast<sha224_ctx*>(state), hash);
#endif
}

/**
 * Calculate SHA2-224 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateSHA224Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
   SHA224(static_cast<const unsigned char*>(data), size, hash);
#else
   sha224_ctx ctx;
   I_sha224_init(&ctx);
   I_sha224_update(&ctx, static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
   I_sha224_final(&ctx, hash);
#endif
}

/**
 * Init SHA256 hash function
 */
void LIBNETXMS_EXPORTABLE SHA256Init(SHA256_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_sha256(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_sha256(), nullptr);
#endif
#else
   I_sha256_init(reinterpret_cast<sha256_ctx*>(state));
#endif
}

/**
 * Append data to SHA256 hash
 */
void LIBNETXMS_EXPORTABLE SHA256Update(SHA256_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_sha256_update(reinterpret_cast<sha256_ctx*>(state), static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish SHA256 calculation
 */
void LIBNETXMS_EXPORTABLE SHA256Final(SHA256_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_sha256_final(reinterpret_cast<sha256_ctx*>(state), hash);
#endif
}


/**
 * Calculate SHA2-256 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateSHA256Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
   SHA256(static_cast<const unsigned char*>(data), size, hash);
#else
   sha256_ctx ctx;
   I_sha256_init(&ctx);
   I_sha256_update(&ctx, static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
   I_sha256_final(&ctx, hash);
#endif
}

/**
 * Init SHA384 hash function
 */
void LIBNETXMS_EXPORTABLE SHA384Init(SHA384_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_sha384(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_sha384(), nullptr);
#endif
#else
   I_sha384_init(reinterpret_cast<sha384_ctx*>(state));
#endif
}

/**
 * Append data to SHA384 hash
 */
void LIBNETXMS_EXPORTABLE SHA384Update(SHA384_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_sha384_update(reinterpret_cast<sha384_ctx*>(state), static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish SHA384 calculation
 */
void LIBNETXMS_EXPORTABLE SHA384Final(SHA384_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_sha384_final(reinterpret_cast<sha384_ctx*>(state), hash);
#endif
}

/**
 * Calculate SHA2-384 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateSHA384Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
   SHA384(static_cast<const unsigned char*>(data), size, hash);
#else
   sha384_ctx ctx;
   I_sha384_init(&ctx);
   I_sha384_update(&ctx, static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
   I_sha384_final(&ctx, hash);
#endif
}

/**
 * Init SHA512 hash function
 */
void LIBNETXMS_EXPORTABLE SHA512Init(SHA512_STATE *state)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   state->context = EVP_MD_CTX_new();
   EVP_DigestInit_ex(state->context, EVP_sha512(), nullptr);
#else
   EVP_MD_CTX_init(&state->context);
   EVP_DigestInit_ex(&state->context, EVP_sha512(), nullptr);
#endif
#else
   I_sha512_init(reinterpret_cast<sha512_ctx*>(state));
#endif
}

/**
 * Append data to SHA512 hash
 */
void LIBNETXMS_EXPORTABLE SHA512Update(SHA512_STATE *state, const void *data, size_t size)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestUpdate(state->context, data, size);
#else
   EVP_DigestUpdate(&state->context, data, size);
#endif
#else
   I_sha512_update(reinterpret_cast<sha512_ctx*>(state), static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
#endif
}

/**
 * Finish SHA512 calculation
 */
void LIBNETXMS_EXPORTABLE SHA512Final(SHA512_STATE *state, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
   EVP_DigestFinal_ex(state->context, hash, nullptr);
   EVP_MD_CTX_free(state->context);
#else
   EVP_DigestFinal_ex(&state->context, hash, nullptr);
   EVP_MD_CTX_cleanup(&state->context);
#endif
#else
   I_sha512_final(reinterpret_cast<sha512_ctx*>(state), hash);
#endif
}

/**
 * Calculate SHA2-512 hash for array of bytes
 */
void LIBNETXMS_EXPORTABLE CalculateSHA512Hash(const void *data, size_t size, BYTE *hash)
{
#ifdef _WITH_ENCRYPTION
   SHA512(static_cast<const unsigned char*>(data), size, hash);
#else
   sha512_ctx ctx;
   I_sha512_init(&ctx);
   I_sha512_update(&ctx, static_cast<const unsigned char*>(data), static_cast<unsigned int>(size));
   I_sha512_final(&ctx, hash);
#endif
}

/**
 * Calculate hash for repeated pattern in virtual buffer using provided hash function
 */
template<typename C, void (*__Init)(C*), void (*__Update)(C*, const void*, size_t), void (*__Final)(C*, BYTE*), size_t __blockSize>
static inline void HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   C context;
   size_t count, patternIndex;
   const unsigned char *src;
   BYTE *dst, patternBuffer[__blockSize];

   __Init(&context);
   for(count = 0, src = static_cast<const unsigned char*>(data), patternIndex = 0; count < fullSize; count += __blockSize)
   {
      dst = patternBuffer;
      for(size_t i = 0; i < __blockSize; i++)
      {
         *dst++ = *src++;
         patternIndex++;
         if (patternIndex >= patternSize)
         {
            patternIndex = 0;
            src = static_cast<const unsigned char*>(data);
         }
      }
      __Update(&context, patternBuffer, __blockSize);
   }
   __Final(&context, hash);
}

/**
 * Calculate MD5 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE MD5HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<MD5_STATE, MD5Init, MD5Update, MD5Final, 64>(data, patternSize, fullSize, hash);
}

/**
 * Calculate SHA1 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE SHA1HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<SHA1_STATE, SHA1Init, SHA1Update, SHA1Final, 64>(data, patternSize, fullSize, hash);
}

/**
 * Calculate SHA224 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE SHA224HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<SHA224_STATE, SHA224Init, SHA224Update, SHA224Final, SHA224_BLOCK_SIZE>(data, patternSize, fullSize, hash);
}

/**
 * Calculate SHA256 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE SHA256HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<SHA256_STATE, SHA256Init, SHA256Update, SHA256Final, SHA256_BLOCK_SIZE>(data, patternSize, fullSize, hash);
}

/**
 * Calculate SHA384 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE SHA384HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<SHA384_STATE, SHA384Init, SHA384Update, SHA384Final, SHA384_BLOCK_SIZE>(data, patternSize, fullSize, hash);
}

/**
 * Calculate SHA224 hash for repeated pattern in virtual buffer
 */
void LIBNETXMS_EXPORTABLE SHA512HashForPattern(const void *data, size_t patternSize, size_t fullSize, BYTE *hash)
{
   HashForPattern<SHA512_STATE, SHA512Init, SHA512Update, SHA512Final, SHA512_BLOCK_SIZE>(data, patternSize, fullSize, hash);
}

/**
 * Calculate hash for given file with provided hash function
 */
template<typename C, void (*__Init)(C*), void (*__Update)(C*, const void*, size_t), void (*__Final)(C*, BYTE*)>
static inline bool CalculateFileHash(const TCHAR *fileName, BYTE *hash, int64_t calculationSize)
{
   bool success = false;
   FILE *fileHandle = _tfopen(fileName, _T("rb"));
   if (fileHandle != nullptr)
   {
      BYTE buffer[FILE_BLOCK_SIZE];
      C context;
      __Init(&context);
      bool useShorterSize = calculationSize > 0;
      while(true)
      {
         size_t size = fread(buffer, 1, useShorterSize ? MIN(calculationSize, FILE_BLOCK_SIZE) : FILE_BLOCK_SIZE , fileHandle);
         if (size == 0 || (useShorterSize && (calculationSize == 0)))
         {
            __Final(&context, hash);
            success = true;
            break;
         }
         __Update(&context, buffer, static_cast<unsigned int>(size));
         calculationSize -= size;
      }
      fclose(fileHandle);
   }
   return success;
}

/**
 * Calculate MD5 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileMD5Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<MD5_STATE, MD5Init, MD5Update, MD5Final>(fileName, hash, size);
}

/**
 * Calculate SHA1 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileSHA1Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<SHA1_STATE, SHA1Init, SHA1Update, SHA1Final>(fileName, hash, size);
}

/**
 * Calculate SHA224 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileSHA224Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<SHA224_STATE, SHA224Init, SHA224Update, SHA224Final>(fileName, hash, size);
}

/**
 * Calculate SHA256 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileSHA256Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<SHA256_STATE, SHA256Init, SHA256Update, SHA256Final>(fileName, hash, size);
}

/**
 * Calculate SHA384 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileSHA384Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<SHA384_STATE, SHA384Init, SHA384Update, SHA384Final>(fileName, hash, size);
}

/**
 * Calculate SHA512 hash for given file
 */
bool LIBNETXMS_EXPORTABLE CalculateFileSHA512Hash(const TCHAR *fileName, BYTE *hash, int64_t size)
{
   return CalculateFileHash<SHA512_STATE, SHA512Init, SHA512Update, SHA512Final>(fileName, hash, size);
}
