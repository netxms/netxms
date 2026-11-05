/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: sshkeys.cpp
**
** Unit tests for ParseOpenSshPrivateKey: structural validation of imported
** OpenSSH private keys and derivation of the public key line.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>
#include <string>
#include <vector>

/**
 * Unencrypted keys generated with ssh-keygen -C "" (the zero padding key with -C "abcde"), the s_pub* strings are
 * the first two fields of ssh-keygen -y output. The sk-ssh-ed25519 key is ed25519_sk1 from OpenSSH
 * regress/unittests/sshkey/testdata.
 */
static const char *s_keyEd25519 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n"
   "QyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQAAAIhQOEmyUDhJ\n"
   "sgAAAAtzc2gtZWQyNTUxOQAAACCOOEUiqU27Ixb0PRXsvAVN2vDzfDmPT5Kdo5epGX1ZrQ\n"
   "AAAEBIrGkjUkDgzigS8krFT+NAXAqoO5+BxCYS7EIn+oV6co44RSKpTbsjFvQ9Fey8BU3a\n"
   "8PN8OY9Pkp2jl6kZfVmtAAAAAAECAwQF\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEd25519 = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAII44RSKpTbsjFvQ9Fey8BU3a8PN8OY9Pkp2jl6kZfVmt";

static const char *s_keyRsa =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAABFwAAAAdzc2gtcn\n"
   "NhAAAAAwEAAQAAAQEA5avZwC6aIWpjJuR3eaAXnxG9bdbGYFs324t3x05QAdN5CsG0w2Lh\n"
   "x0D1Z5QrCj1dymqr0Nn4CZUjYCRshGwn0OgFn53QPVWliZGai1W9jf7VPrC6wGiVylZINZ\n"
   "dWcu5bH5tH3dAXZ01Ry6sJN98AGTOIRA1hiAcRr/p2CyedkL57+4YopOlemR3q9PXf0YH4\n"
   "YmzNJ4FygRS98yobHtL4qsICO3NoSCW5qCBnVQwxk6Exw+frKMSmgWysMPP0/B/6tNmLCE\n"
   "ouGF5FBfKkWUrSlAIgMfx7FjXhaTgdJRHnDHYUrZ/WflMDFzSGlh9vHCMvgMvfOmGvVSPY\n"
   "YZiT/edRnwAAA7jL4Wxly+FsZQAAAAdzc2gtcnNhAAABAQDlq9nALpohamMm5Hd5oBefEb\n"
   "1t1sZgWzfbi3fHTlAB03kKwbTDYuHHQPVnlCsKPV3KaqvQ2fgJlSNgJGyEbCfQ6AWfndA9\n"
   "VaWJkZqLVb2N/tU+sLrAaJXKVkg1l1Zy7lsfm0fd0BdnTVHLqwk33wAZM4hEDWGIBxGv+n\n"
   "YLJ52Qvnv7hiik6V6ZHer09d/RgfhibM0ngXKBFL3zKhse0viqwgI7c2hIJbmoIGdVDDGT\n"
   "oTHD5+soxKaBbKww8/T8H/q02YsISi4YXkUF8qRZStKUAiAx/HsWNeFpOB0lEecMdhStn9\n"
   "Z+UwMXNIaWH28cIy+Ay986Ya9VI9hhmJP951GfAAAAAwEAAQAAAQA4ek4NyGf2l6bSRZRr\n"
   "wEaOw1oFzK7zuvq9bkqCJGvgRxSY5JGDCqniycKPOgxrdDPANwMRVMh98sLEPbDBz6bq5G\n"
   "OdoL04l/shHUEXynnWlTfzNpp1/kH3g8g2BYUothyKZTEDBsz6hjT7FJArwRbRG68CrRey\n"
   "z6o10C1/CLrigL2yg7+/94RFaC/8ZRZ2Nbno/YwIhYIi0YJD5a1K3q4OmWbtgoZPkBlb3Z\n"
   "68EWqOzFHiUyfhrw2VD6ui/NLH2wcwlljrmyzBp5inCFgVK7Tz0SouZ2guDtrXFwe0kLFS\n"
   "S6R72lZ8N4Y33wn76GfmDH9PwThyJqJ/hbfO1mjTGsLhAAAAgFH4DtRFdgVEEkHtRemJmO\n"
   "ejE3qgOydESIlqoMAMi730hQBO2QPrp06euIX4kXHS9tBVGGzKk/fyE+9p5FPv8Z4cWx+w\n"
   "Mgthvgfs8CrE0vsS/MEj2xDHxbXmGlpa2BbAs17YTbERqhqhNIJ+EFg278jbGgEczaFsPJ\n"
   "ntFlk98nCYAAAAgQD94sBs8ev+zYg7JmH5Xe+3gL9isl5AZrePftSNL3BxZnbSykI7CTbX\n"
   "wZ9z7jZae2bFoGu8TViOmSQ44GDR/WloGMcPFyETj5olSTv7LNsqX1ykDPh3kvVwvCrfHM\n"
   "CvHsMKQJVRPoD8xQgsZWtXBKn/EgW2r8npz8jxm0as7XaUKwAAAIEA55V6JiHQYnMA60VA\n"
   "jgQOiNQWryydVvNbQhDVH14yqyVsEw6u3EN4PWXKCxM3E8V0S8FM3+2MQYYYRsuEHQLxC0\n"
   "bnsr7V/QnvnJ0eXv/FdwikwRd04dBG1+WCLICHAZ5SzErgKom/j0IDqFxA5n30wGiqc7fp\n"
   "N/wLCwv8w+clel0AAAAAAQID\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubRsa = "ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDlq9nALpohamMm5Hd5oBefEb1t1sZgWzfbi3fHTlAB03kKwbTDYuHHQPVnlCsKPV3KaqvQ2fgJlSNgJGyEbCfQ6AWfndA9VaWJkZqLVb2N/tU+sLrAaJXKVkg1l1Zy7lsfm0fd0BdnTVHLqwk33wAZM4hEDWGIBxGv+nYLJ52Qvnv7hiik6V6ZHer09d/RgfhibM0ngXKBFL3zKhse0viqwgI7c2hIJbmoIGdVDDGToTHD5+soxKaBbKww8/T8H/q02YsISi4YXkUF8qRZStKUAiAx/HsWNeFpOB0lEecMdhStn9Z+UwMXNIaWH28cIy+Ay986Ya9VI9hhmJP951Gf";

static const char *s_keyEcdsa256 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAaAAAABNlY2RzYS\n"
   "1zaGEyLW5pc3RwMjU2AAAACG5pc3RwMjU2AAAAQQToImQDkuikLRiAJ8gyznG/2eARyRg0\n"
   "Eq4Z2KjHuuR/Bg+VLiUXemo/Gt/Dju/Qw1FpO90EqxCq+GQoyGAtTN44AAAAmCexuYonsb\n"
   "mKAAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOgiZAOS6KQtGIAn\n"
   "yDLOcb/Z4BHJGDQSrhnYqMe65H8GD5UuJRd6aj8a38OO79DDUWk73QSrEKr4ZCjIYC1M3j\n"
   "gAAAAgMnVFzX3CHCo36mZt5UDt2pXl1DmtN/kIdovmxo2/K/8AAAAA\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEcdsa256 = "ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBOgiZAOS6KQtGIAnyDLOcb/Z4BHJGDQSrhnYqMe65H8GD5UuJRd6aj8a38OO79DDUWk73QSrEKr4ZCjIYC1M3jg=";

static const char *s_keyEcdsa384 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAiAAAABNlY2RzYS\n"
   "1zaGEyLW5pc3RwMzg0AAAACG5pc3RwMzg0AAAAYQRumT6a/+xzrc9RpmmimCYJw+vSLCj6\n"
   "9wRkZlq5BH01iWssGSwqvOOUbbkk/cYvKPBLL+7UmUmwNxTItuU16tT2TPp9MhBHQvVwb0\n"
   "gU5n42Q81koOriUXiN20r70BgxJLIAAADQHP2LtBz9i7QAAAATZWNkc2Etc2hhMi1uaXN0\n"
   "cDM4NAAAAAhuaXN0cDM4NAAAAGEEbpk+mv/sc63PUaZpopgmCcPr0iwo+vcEZGZauQR9NY\n"
   "lrLBksKrzjlG25JP3GLyjwSy/u1JlJsDcUyLblNerU9kz6fTIQR0L1cG9IFOZ+NkPNZKDq\n"
   "4lF4jdtK+9AYMSSyAAAAMQDus9KXj212Bfe4jq7tG674QoMw5wMLKgFF12AriB4PjP56N2\n"
   "sfae2f6q0r1DUVIOkAAAAAAQIDBAUGBw==\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEcdsa384 = "ecdsa-sha2-nistp384 AAAAE2VjZHNhLXNoYTItbmlzdHAzODQAAAAIbmlzdHAzODQAAABhBG6ZPpr/7HOtz1GmaaKYJgnD69IsKPr3BGRmWrkEfTWJaywZLCq845RtuST9xi8o8Esv7tSZSbA3FMi25TXq1PZM+n0yEEdC9XBvSBTmfjZDzWSg6uJReI3bSvvQGDEksg==";

static const char *s_keyEcdsa521 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAArAAAABNlY2RzYS\n"
   "1zaGEyLW5pc3RwNTIxAAAACG5pc3RwNTIxAAAAhQQBVtLnAaiqfiQRRg4xLJSi+Q8mMOU0\n"
   "UY5V9gEBnj2hc7NA9QTkuMXVMXdkTh3U5zCkdhfncIsM5OMogRSom6Jih8kA8a403vQPN9\n"
   "A9piwv2b8pweZA0EWMrLaHNgE33lwixj4HcnZxUkEgW5s76CUhhfLAQbSvPnlNc6FJw6iz\n"
   "NsezhgwAAAEA085yqdPOcqkAAAATZWNkc2Etc2hhMi1uaXN0cDUyMQAAAAhuaXN0cDUyMQ\n"
   "AAAIUEAVbS5wGoqn4kEUYOMSyUovkPJjDlNFGOVfYBAZ49oXOzQPUE5LjF1TF3ZE4d1Ocw\n"
   "pHYX53CLDOTjKIEUqJuiYofJAPGuNN70DzfQPaYsL9m/KcHmQNBFjKy2hzYBN95cIsY+B3\n"
   "J2cVJBIFubO+glIYXywEG0rz55TXOhScOoszbHs4YMAAAAQgD2murc8s136OWmS1/WHNrd\n"
   "i/xgN2IxUv1IbxH/4SGa8Hf7k0H/0RtAJ7YlCC9w5Hi9FwTqOhsS3DMSd/9vlXBw2gAAAA\n"
   "ABAg==\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEcdsa521 = "ecdsa-sha2-nistp521 AAAAE2VjZHNhLXNoYTItbmlzdHA1MjEAAAAIbmlzdHA1MjEAAACFBAFW0ucBqKp+JBFGDjEslKL5DyYw5TRRjlX2AQGePaFzs0D1BOS4xdUxd2ROHdTnMKR2F+dwiwzk4yiBFKibomKHyQDxrjTe9A830D2mLC/ZvynB5kDQRYystoc2ATfeXCLGPgdydnFSQSBbmzvoJSGF8sBBtK8+eU1zoUnDqLM2x7OGDA==";

static const char *s_keyEd25519ZeroPadding =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW\n"
   "QyNTUxOQAAACD0AwRc4B4ghs8SPaBt3YHPlrtYVk9S74ELRoVCC6VQyQAAAIg1ymp8Ncpq\n"
   "fAAAAAtzc2gtZWQyNTUxOQAAACD0AwRc4B4ghs8SPaBt3YHPlrtYVk9S74ELRoVCC6VQyQ\n"
   "AAAED/edLZQEi4NppJlpCwjVOB7ipN1l09wQcCqTJpjRJuI/QDBFzgHiCGzxI9oG3dgc+W\n"
   "u1hWT1LvgQtGhUILpVDJAAAABWFiY2Rl\n"
   "-----END OPENSSH PRIVATE KEY-----\n";
static const char *s_pubEd25519ZeroPadding = "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIPQDBFzgHiCGzxI9oG3dgc+Wu1hWT1LvgQtGhUILpVDJ";

static const char *s_keyEncrypted =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAACmFlczI1Ni1jdHIAAAAGYmNyeXB0AAAAGAAAABCPkdBWFF\n"
   "GYbvnGTzb9u/B1AAAAGAAAAAEAAAAzAAAAC3NzaC1lZDI1NTE5AAAAIC5+qFVNQ6QsAXl3\n"
   "17Q90riC2frJSh9ud18KBKFuGtNAAAAAkD82ax3h184h/nQqNaz534aLFrlxpM/tq4mPDJ\n"
   "mDUYe02bh1KzuxoDpqLyJXsMvQBRvL9mVh9uzp6U/ofXYtc2dEA0fNjDZZLjEtFzIWdFfp\n"
   "8WfY5zwKSkiJejaNfCK6nE30a+3+mu+PrrasmhysMefPKc9xAFqrAiF9floN7JaAtlq6Os\n"
   "ml45CPFZ8rLE9WSQ==\n"
   "-----END OPENSSH PRIVATE KEY-----\n";

static const char *s_keyPemRsa =
   "-----BEGIN RSA PRIVATE KEY-----\n"
   "MIICXAIBAAKBgQCcR/kUna8VEDdd8WGMOaU8FkQKO4bvNyZkfNUVstYZmWGnH7I1\n"
   "Z6rRDui5X/DUdQpcyZNs63678pZaKRfu+JRvCOh589DmWQ/5tFbcfoWoCCAZL9Yl\n"
   "km5S+M4zOcq7Fw3Libus9hJtaexQVyDbdJrmncrr8E/0QtINrnk3CL6YVwIDAQAB\n"
   "AoGAETn9wr/+5Jt8/uX0krhn9k+eAGK/ZO73VGxOMK4d24SEtx9PTYO32IOEM6rX\n"
   "qROdqMyva3op5h2/ZQDSmozcD5UbHBNUzeoqkgJSuhY42FxatyZHqbs+0FG1syji\n"
   "J4ZwdQmUcRtWQY2ogp2r1TF+nELoS20fGL7LJtiiukhOI8ECQQDIqpSospyeDLin\n"
   "cTBlfEpVvgecXn4NaeCuwL42/SmCASX11hD8HVL78jWr3aQbw+YUAR/IYvTqdTz5\n"
   "kHAtQ/8bAkEAx2AoQSomnPHQC0/UCZ+rwiZo281FU2K5tmtoBSqKwdfK7W4pKr3u\n"
   "WTVYiUwvyYmvr+Y029vHkg7vweT85fQTdQJATUwQOa+bzvH9i1p5lw46oC60RdLM\n"
   "YaRgTVCntQzuGUsR8VHFdyhgl/akIBEfryZWEWyhbpJX35zLoSK3Y2hm8wJBAJVb\n"
   "68wlj5ezb2+J9gbfnwRrUdKYDH9TTlziSaLVI+3jgxorlJt4fb5GwHTaplIM25ii\n"
   "lKbAFtnMZsCSkwKRvqECQB1dRsa7FBFo/6mre897lwpKhMfNCVGQ+Fn1Alzf9Q+R\n"
   "Eg64aCKf6NMnKjJ730ivF/r8mxGu3M8vFz9ltJ9F6ns=\n"
   "-----END RSA PRIVATE KEY-----\n";

static const char *s_keySkEd25519 =
   "-----BEGIN OPENSSH PRIVATE KEY-----\n"
   "b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAASgAAABpzay1zc2\n"
   "gtZWQyNTUxOUBvcGVuc3NoLmNvbQAAACAhaP5OS1PPOt7uumAvXlDtte9EHbqIT1EZEJ2y\n"
   "2v3XMwAAAARzc2g6AAAAuBocY6UaHGOlAAAAGnNrLXNzaC1lZDI1NTE5QG9wZW5zc2guY2\n"
   "9tAAAAICFo/k5LU8863u66YC9eUO2170QduohPURkQnbLa/dczAAAABHNzaDoBAAAAQJYq\n"
   "lGHhFoA25/q8X/rdTqDAb7dhqs4ehhd/w8x99CwiIWj+TktTzzre7rpgL15Q7bXvRB26iE\n"
   "9RGRCdstr91zMAAAAAAAAAFkVEMjU1MTktU0sgdGVzdCBrZXkgIzEBAgM=\n"
   "-----END OPENSSH PRIVATE KEY-----\n";


static const wchar_t *s_keyName = L"test key ü";
static const char *s_keyNameUtf8 = "test key \xC3\xBC";

/**
 * Decoded OpenSSH key container, re-encoded by the mutation tests
 */
struct OpenSshKey
{
   std::string cipher;
   std::string kdf;
   std::string kdfOptions;
   uint32_t keyCount;
   std::vector<std::string> publicFields;
   std::string publicTrailer;
   uint32_t check1;
   uint32_t check2;
   std::vector<std::string> privateFields;
   bool hasComment;
   std::string comment;
   bool autoPadding;
   std::string padding;
   std::string trailer;
};

/**
 * Read big-endian uint32 from decoded key data
 */
static uint32_t GetUInt32(const std::string& data, size_t *pos)
{
   const BYTE *p = reinterpret_cast<const BYTE*>(data.data()) + *pos;
   *pos += 4;
   return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) | (static_cast<uint32_t>(p[2]) << 8) | p[3];
}

/**
 * Read length-prefixed string from decoded key data
 */
static std::string GetString(const std::string& data, size_t *pos)
{
   uint32_t len = GetUInt32(data, pos);
   std::string s = data.substr(*pos, len);
   *pos += len;
   return s;
}

/**
 * Append big-endian uint32
 */
static void PutUInt32(std::string *out, uint32_t value)
{
   out->push_back(static_cast<char>(value >> 24));
   out->push_back(static_cast<char>(value >> 16));
   out->push_back(static_cast<char>(value >> 8));
   out->push_back(static_cast<char>(value));
}

/**
 * Append length-prefixed string
 */
static void PutString(std::string *out, const std::string& s)
{
   PutUInt32(out, static_cast<uint32_t>(s.size()));
   out->append(s);
}

/**
 * Decode armored fixture into raw key data
 */
static std::string Unarmor(const char *text)
{
   std::string body;
   const char *p = strchr(text, '\n') + 1;
   const char *end = strstr(p, "-----END");
   for(; p < end; p++)
      if (*p != '\n')
         body.push_back(*p);
   char *decoded;
   size_t len;
   base64_decode_alloc(body.data(), body.size(), &decoded, &len);
   std::string data(decoded, len);
   MemFree(decoded);
   return data;
}

/**
 * Armor raw key data the way ssh-keygen does (70 column lines, trailing newline)
 */
static std::string Armor(const std::string& data)
{
   char *encoded;
   size_t len = base64_encode_alloc(data.data(), data.size(), &encoded);
   std::string text = "-----BEGIN OPENSSH PRIVATE KEY-----\n";
   for(size_t i = 0; i < len; i += 70)
   {
      text.append(encoded + i, std::min<size_t>(70, len - i));
      text.push_back('\n');
   }
   text.append("-----END OPENSSH PRIVATE KEY-----\n");
   MemFree(encoded);
   return text;
}

/**
 * Decode fixture into its fields. Key field count includes the key type field.
 */
static OpenSshKey DecodeKey(const char *text, int privateKeyFieldCount)
{
   std::string data = Unarmor(text);
   OpenSshKey key;
   size_t pos = 15;  // magic
   key.cipher = GetString(data, &pos);
   key.kdf = GetString(data, &pos);
   key.kdfOptions = GetString(data, &pos);
   key.keyCount = GetUInt32(data, &pos);

   std::string publicBlob = GetString(data, &pos);
   for(size_t p = 0; p < publicBlob.size();)
      key.publicFields.push_back(GetString(publicBlob, &p));

   std::string privateSection = GetString(data, &pos);
   size_t p = 0;
   key.check1 = GetUInt32(privateSection, &p);
   key.check2 = GetUInt32(privateSection, &p);
   for(int i = 0; i < privateKeyFieldCount; i++)
      key.privateFields.push_back(GetString(privateSection, &p));
   key.hasComment = true;
   key.comment = GetString(privateSection, &p);
   key.autoPadding = true;
   return key;
}

/**
 * Encode key fields into raw key data
 */
static std::string EncodeKey(const OpenSshKey& key)
{
   std::string data("openssh-key-v1", 15);
   PutString(&data, key.cipher);
   PutString(&data, key.kdf);
   PutString(&data, key.kdfOptions);
   PutUInt32(&data, key.keyCount);

   std::string publicBlob;
   for(const std::string& f : key.publicFields)
      PutString(&publicBlob, f);
   publicBlob.append(key.publicTrailer);
   PutString(&data, publicBlob);

   std::string privateSection;
   PutUInt32(&privateSection, key.check1);
   PutUInt32(&privateSection, key.check2);
   for(const std::string& f : key.privateFields)
      PutString(&privateSection, f);
   if (key.hasComment)
      PutString(&privateSection, key.comment);
   if (key.autoPadding)
   {
      for(char i = 1; privateSection.size() % 8 != 0; i++)
         privateSection.push_back(i);
   }
   else
   {
      privateSection.append(key.padding);
   }
   PutString(&data, privateSection);
   data.append(key.trailer);
   return data;
}

/**
 * Check if parser accepts given text
 */
static bool Accepts(const std::string& text)
{
   char *privateKey = nullptr, *publicKey = nullptr;
   bool success = ParseOpenSshPrivateKey(text.c_str(), s_keyName, &privateKey, &publicKey);
   MemFree(privateKey);
   MemFree(publicKey);
   return success;
}

/**
 * Check if parser accepts key built from given fields
 */
static bool Accepts(const OpenSshKey& key)
{
   return Accepts(Armor(EncodeKey(key)));
}

/**
 * Parse valid fixture and check derived public key and normalized private key text
 */
static void CheckValidKey(const char *text, const char *expectedTypeAndBlob)
{
   char *privateKey, *publicKey;
   AssertTrue(ParseOpenSshPrivateKey(text, s_keyName, &privateKey, &publicKey));
   AssertEquals(publicKey, (std::string(expectedTypeAndBlob) + " " + s_keyNameUtf8).c_str());
   AssertEquals(privateKey, std::string(text, strlen(text) - 1).c_str());
   MemFree(privateKey);
   MemFree(publicKey);
}

/**
 * All supported key types and the zero padding case
 */
static void TestValidKeys()
{
   StartTest(_T("OpenSSH key parser - valid keys"));
   CheckValidKey(s_keyEd25519, s_pubEd25519);
   CheckValidKey(s_keyRsa, s_pubRsa);
   CheckValidKey(s_keyEcdsa256, s_pubEcdsa256);
   CheckValidKey(s_keyEcdsa384, s_pubEcdsa384);
   CheckValidKey(s_keyEcdsa521, s_pubEcdsa521);
   CheckValidKey(s_keyEd25519ZeroPadding, s_pubEd25519ZeroPadding);
   EndTest();
}

/**
 * Test key builder reproduces the fixtures byte for byte
 */
static void TestKeyBuilder()
{
   StartTest(_T("OpenSSH key parser - test key builder"));
   AssertEquals(Armor(EncodeKey(DecodeKey(s_keyEd25519, 3))).c_str(), s_keyEd25519);
   AssertEquals(Armor(EncodeKey(DecodeKey(s_keyEd25519ZeroPadding, 3))).c_str(), s_keyEd25519ZeroPadding);
   AssertEquals(Armor(EncodeKey(DecodeKey(s_keyRsa, 7))).c_str(), s_keyRsa);
   AssertEquals(Armor(EncodeKey(DecodeKey(s_keyEcdsa384, 4))).c_str(), s_keyEcdsa384);
   EndTest();
}

/**
 * Replace every occurrence of a substring
 */
static std::string ReplaceAll(std::string s, const std::string& from, const std::string& to)
{
   for(size_t pos = s.find(from); pos != std::string::npos; pos = s.find(from, pos + to.size()))
      s.replace(pos, from.size(), to);
   return s;
}

/**
 * CRLF line endings and surrounding whitespace are normalized away
 */
static void TestNormalization()
{
   StartTest(_T("OpenSSH key parser - normalization"));

   std::string expected(s_keyEd25519, strlen(s_keyEd25519) - 1);

   std::string crlf = std::string(" \t\r\n\r\n") + ReplaceAll(s_keyEd25519, "\n", "\r\n") + "\r\n \t\n";
   char *privateKey, *publicKey;
   AssertTrue(ParseOpenSshPrivateKey(crlf.c_str(), s_keyName, &privateKey, &publicKey));
   AssertEquals(privateKey, expected.c_str());
   AssertEquals(publicKey, (std::string(s_pubEd25519) + " " + s_keyNameUtf8).c_str());
   MemFree(privateKey);
   MemFree(publicKey);

   AssertTrue(ParseOpenSshPrivateKey(expected.c_str(), s_keyName, &privateKey, &publicKey));
   AssertEquals(privateKey, expected.c_str());
   MemFree(privateKey);
   MemFree(publicKey);

   EndTest();
}

/**
 * Formats and key types outside the supported set
 */
static void TestUnsupportedKeys()
{
   StartTest(_T("OpenSSH key parser - unsupported keys"));
   AssertFalse(Accepts(s_keyEncrypted));
   AssertFalse(Accepts(s_keyPemRsa));
   AssertFalse(Accepts(s_keySkEd25519));

   OpenSshKey key = DecodeKey(s_keyEd25519, 3);
   key.publicFields[0] = key.privateFields[0] = "ssh-ed25519-cert-v01@openssh.com";
   AssertFalse(Accepts(key));
   key.publicFields[0] = key.privateFields[0] = "ssh-dss";
   AssertFalse(Accepts(key));
   EndTest();
}

/**
 * Armor and base64 body
 */
static void TestArmor()
{
   StartTest(_T("OpenSSH key parser - armor"));

   std::string valid(s_keyEd25519);
   AssertFalse(Accepts(""));
   AssertFalse(Accepts("hello"));
   AssertFalse(Accepts("-----BEGIN OPENSSH PRIVATE KEY-----\n-----END OPENSSH PRIVATE KEY-----\n"));
   AssertFalse(Accepts("-----BEGIN OPENSSH PRIVATE KEY-----\n"));
   AssertFalse(Accepts(valid + "garbage"));
   AssertFalse(Accepts(valid + valid));
   AssertFalse(Accepts("garbage\n" + valid));
   AssertFalse(Accepts(ReplaceAll(valid, "BEGIN OPENSSH", "BEGIN  OPENSSH")));
   AssertFalse(Accepts(ReplaceAll(valid, "-----END OPENSSH PRIVATE KEY-----", "-----END OPENSSH PRIVATE KEY----")));
   AssertFalse(Accepts(ReplaceAll(valid, "-----BEGIN OPENSSH PRIVATE KEY-----", "-----begin openssh private key-----")));

   // non-alphabet characters inside the body
   AssertFalse(Accepts(ReplaceAll(valid, "b3BlbnNz", "b3Bl*nNz")));
   AssertFalse(Accepts(ReplaceAll(valid, "b3BlbnNz", "b3Bl nNz")));
   AssertFalse(Accepts(ReplaceAll(valid, "b3BlbnNz", "b3Bl\tnNz")));
   AssertFalse(Accepts(ReplaceAll(valid, "\nQyNTUx", "\n\nQyNTUx")));
   AssertFalse(Accepts(ReplaceAll(valid, "\nQyNTUx", "\rQyNTUx")));

   // base64 length and padding
   AssertFalse(Accepts(ReplaceAll(valid, "ECAwQF\n", "ECAwQ\n")));
   AssertFalse(Accepts(ReplaceAll(valid, "ECAwQF\n", "ECAwQF=\n")));
   AssertFalse(Accepts(ReplaceAll(valid, "b3BlbnNz", "b3Bl=nNz")));

   EndTest();
}

/**
 * Input size limit is applied to the raw input
 */
static void TestSizeLimit()
{
   StartTest(_T("OpenSSH key parser - size limit"));

   AssertFalse(Accepts(std::string(s_keyEd25519) + std::string(16384, ' ')));

   OpenSshKey key = DecodeKey(s_keyEd25519, 3);
   key.comment = std::string(11000, 'c');
   std::string text = Armor(EncodeKey(key));
   AssertTrue(text.size() <= 16384);
   AssertTrue(Accepts(text));

   key.comment = std::string(13000, 'c');
   text = Armor(EncodeKey(key));
   AssertTrue(text.size() > 16384);
   AssertFalse(Accepts(text));

   EndTest();
}

/**
 * Every prefix of a valid key is rejected, as are length fields that run past the data
 */
static void TestTruncation()
{
   StartTest(_T("OpenSSH key parser - truncation"));

   std::string data = EncodeKey(DecodeKey(s_keyEd25519, 3));
   for(size_t len = 0; len < data.size(); len++)
      AssertFalse(Accepts(Armor(data.substr(0, len))));

   std::string huge("openssh-key-v1", 15);
   PutUInt32(&huge, 0xFFFFFFFF);
   huge.append("none");
   AssertFalse(Accepts(Armor(huge)));

   huge.resize(15);
   PutUInt32(&huge, 0xFFFFFFFC);
   huge.append("none");
   AssertFalse(Accepts(Armor(huge)));

   EndTest();
}

/**
 * Envelope fields
 */
static void TestEnvelope()
{
   StartTest(_T("OpenSSH key parser - envelope"));

   const OpenSshKey valid = DecodeKey(s_keyEd25519, 3);
   AssertTrue(Accepts(valid));

   std::string data = EncodeKey(valid);
   data[0] = 'O';
   AssertFalse(Accepts(Armor(data)));
   data = EncodeKey(valid);
   data[14] = '1';
   AssertFalse(Accepts(Armor(data)));

   OpenSshKey key = valid;
   key.cipher = "aes256-ctr";
   AssertFalse(Accepts(key));

   key = valid;
   key.cipher = std::string("none\0x", 6);
   AssertFalse(Accepts(key));

   key = valid;
   key.cipher = "non";
   AssertFalse(Accepts(key));

   key = valid;
   key.kdf = "bcrypt";
   AssertFalse(Accepts(key));

   key = valid;
   key.kdf = std::string("none\0", 5);
   AssertFalse(Accepts(key));

   key = valid;
   key.kdfOptions = "x";
   AssertFalse(Accepts(key));

   key = valid;
   key.keyCount = 0;
   AssertFalse(Accepts(key));
   key.keyCount = 2;
   AssertFalse(Accepts(key));

   key = valid;
   key.publicTrailer = std::string(1, '\0');
   AssertFalse(Accepts(key));

   key = valid;
   key.trailer = std::string(1, '\0');
   AssertFalse(Accepts(key));

   EndTest();
}

/**
 * Private section framing: check integers, type, comment and padding
 */
static void TestPrivateSection()
{
   StartTest(_T("OpenSSH key parser - private section"));

   const OpenSshKey valid = DecodeKey(s_keyEd25519, 3);

   OpenSshKey key = valid;
   key.check2 ^= 1;
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[0] = "ssh-rsa";
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[0] = std::string("ssh-ed25519\0", 12);
   AssertFalse(Accepts(key));

   key = valid;
   key.hasComment = false;
   AssertFalse(Accepts(key));

   // fixture needs 5 padding bytes
   key = valid;
   key.autoPadding = false;
   key.padding = "\x01\x02\x03\x04\x05";
   AssertTrue(Accepts(key));
   key.padding = "\x01\x02\x03\x04\x06";
   AssertFalse(Accepts(key));
   key.padding = "\x00\x02\x03\x04\x05";
   AssertFalse(Accepts(key));
   key.padding = "\x01\x02\x03\x04";
   AssertFalse(Accepts(key));
   key.padding = "\x01\x02\x03\x04\x05\x06";
   AssertFalse(Accepts(key));

   key = DecodeKey(s_keyEd25519ZeroPadding, 3);
   key.autoPadding = false;
   key.padding = "";
   AssertTrue(Accepts(key));
   key.padding = "\x01";
   AssertFalse(Accepts(key));

   EndTest();
}

/**
 * Ed25519 field sizes and consistency between sections
 */
static void TestEd25519Structure()
{
   StartTest(_T("OpenSSH key parser - Ed25519 structure"));

   const OpenSshKey valid = DecodeKey(s_keyEd25519, 3);
   const std::string seed = valid.privateFields[2].substr(0, 32);

   OpenSshKey key = valid;
   key.publicFields[1].resize(31);
   key.privateFields[1] = key.publicFields[1];
   key.privateFields[2] = seed + key.publicFields[1];
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[1].push_back('\x01');
   key.privateFields[1] = key.publicFields[1];
   key.privateFields[2] = seed + key.publicFields[1];
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[2].resize(63);
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[2].push_back('\x01');
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[2][40] ^= 1;
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[1][0] ^= 1;
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[1][0] ^= 1;
   key.privateFields[2][32] ^= 1;
   AssertFalse(Accepts(key));

   EndTest();
}

/**
 * Prepend redundant zero byte to canonical mpint
 */
static std::string AddRedundantZero(const std::string& mpint)
{
   return std::string(1, '\0') + mpint;
}

/**
 * Make canonical positive mpint negative
 */
static std::string MakeNegative(std::string mpint)
{
   if (mpint[0] == 0)
      return mpint.substr(1);
   mpint[0] |= 0x80;
   return mpint;
}

/**
 * RSA mpints and consistency between sections
 */
static void TestRsaStructure()
{
   StartTest(_T("OpenSSH key parser - RSA structure"));

   // envelope: type, e, n; private: type, n, e, d, iqmp, p, q
   const OpenSshKey valid = DecodeKey(s_keyRsa, 7);
   AssertTrue(Accepts(valid));

   OpenSshKey key = valid;
   key.publicFields[1] = key.privateFields[2] = AddRedundantZero(valid.publicFields[1]);
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[2] = key.privateFields[1] = AddRedundantZero(valid.publicFields[2]);
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[2] = key.privateFields[1] = MakeNegative(valid.publicFields[2]);
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[1] = key.privateFields[2] = "";
   AssertFalse(Accepts(key));

   for(int i = 3; i <= 6; i++)
   {
      key = valid;
      key.privateFields[i] = AddRedundantZero(valid.privateFields[i]);
      AssertFalse(Accepts(key));
      key.privateFields[i] = MakeNegative(valid.privateFields[i]);
      AssertFalse(Accepts(key));
      key.privateFields[i] = "";
      AssertFalse(Accepts(key));
   }

   key = valid;
   key.privateFields[1][key.privateFields[1].size() - 1] ^= 1;
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[2] = "\x03";
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields.pop_back();
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields.pop_back();
   AssertFalse(Accepts(key));

   EndTest();
}

/**
 * ECDSA curve names, points and private scalar
 */
static void TestEcdsaStructure()
{
   StartTest(_T("OpenSSH key parser - ECDSA structure"));

   // envelope: type, curve, Q; private: type, curve, Q, scalar
   const OpenSshKey valid = DecodeKey(s_keyEcdsa256, 4);
   AssertTrue(Accepts(valid));

   OpenSshKey key = valid;
   key.publicFields[1] = "nistp384";
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[1] = "nistp384";
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[1] = key.privateFields[1] = std::string("nistp256\0", 9);
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[2] = key.privateFields[2] = std::string(1, '\x02') + valid.publicFields[2].substr(1, 32);
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[2][0] = key.privateFields[2][0] = '\x03';
   AssertFalse(Accepts(key));

   key = valid;
   key.publicFields[2].pop_back();
   key.privateFields[2] = key.publicFields[2];
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[2][64] ^= 1;
   AssertFalse(Accepts(key));

   key = valid;
   key.privateFields[3] = AddRedundantZero(valid.privateFields[3]);
   AssertFalse(Accepts(key));
   key.privateFields[3] = MakeNegative(valid.privateFields[3]);
   AssertFalse(Accepts(key));
   key.privateFields[3] = "";
   AssertFalse(Accepts(key));

   key = DecodeKey(s_keyEcdsa521, 4);
   key.publicFields[0] = key.privateFields[0] = "ecdsa-sha2-nistp384";
   AssertFalse(Accepts(key));

   EndTest();
}

/**
 * Entry point
 */
void TestOpenSshKeyParser()
{
   TestKeyBuilder();
   TestValidKeys();
   TestNormalization();
   TestUnsupportedKeys();
   TestArmor();
   TestSizeLimit();
   TestTruncation();
   TestEnvelope();
   TestPrivateSection();
   TestEd25519Structure();
   TestRsaStructure();
   TestEcdsaStructure();
}
