/*
** NetXMS - Network Management System
** Copyright (C) 2024-2026 Raden Solutions
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
** File: otlp_attributes.h
**
** Shared helpers for flattening OTLP AnyValue / KeyValue collections.
**
**/

#ifndef _otlp_attributes_h_
#define _otlp_attributes_h_

#include "generated/common.pb.h"
#include "generated/resource.pb.h"

/**
 * Convert raw bytes to lowercase hex string
 */
static inline std::string OtlpBytesToHex(const std::string& bytes)
{
   static const char hexChars[] = "0123456789abcdef";
   std::string out;
   out.reserve(bytes.size() * 2);
   for (char c : bytes)
   {
      uint8_t b = static_cast<uint8_t>(c);
      out.push_back(hexChars[b >> 4]);
      out.push_back(hexChars[b & 0x0F]);
   }
   return out;
}

/**
 * Convert OTLP AnyValue to JSON. Bytes are encoded as lowercase hex string, empty value as null.
 */
static inline json_t *AnyValueToJson(const opentelemetry::proto::common::v1::AnyValue& av)
{
   switch (av.value_case())
   {
      case opentelemetry::proto::common::v1::AnyValue::kStringValue:
         return json_stringn_nocheck(av.string_value().data(), av.string_value().size());
      case opentelemetry::proto::common::v1::AnyValue::kIntValue:
         return json_integer(av.int_value());
      case opentelemetry::proto::common::v1::AnyValue::kDoubleValue:
      {
         json_t *value = json_real(av.double_value());   // returns nullptr for NaN and infinity
         return (value != nullptr) ? value : json_null();
      }
      case opentelemetry::proto::common::v1::AnyValue::kBoolValue:
         return json_boolean(av.bool_value());
      case opentelemetry::proto::common::v1::AnyValue::kBytesValue:
      {
         std::string hex = OtlpBytesToHex(av.bytes_value());
         return json_stringn_nocheck(hex.data(), hex.size());
      }
      case opentelemetry::proto::common::v1::AnyValue::kArrayValue:
      {
         json_t *array = json_array();
         for (const auto& element : av.array_value().values())
            json_array_append_new(array, AnyValueToJson(element));
         return array;
      }
      case opentelemetry::proto::common::v1::AnyValue::kKvlistValue:
      {
         json_t *object = json_object();
         for (const auto& kv : av.kvlist_value().values())
            json_object_set_new_nocheck(object, kv.key().c_str(), AnyValueToJson(kv.value()));
         return object;
      }
      default:
         return json_null();
   }
}

/**
 * Convert a single OTLP AnyValue to its string representation. Arrays and key-value lists
 * are rendered as compact JSON, bytes as lowercase hex string, empty value as empty string.
 */
static inline void AnyValueToString(const opentelemetry::proto::common::v1::AnyValue& av, std::string& out)
{
   char buffer[32];
   switch (av.value_case())
   {
      case opentelemetry::proto::common::v1::AnyValue::kStringValue:
         out = av.string_value();
         break;
      case opentelemetry::proto::common::v1::AnyValue::kIntValue:
         IntegerToString(av.int_value(), buffer);
         out = buffer;
         break;
      case opentelemetry::proto::common::v1::AnyValue::kDoubleValue:
         snprintf(buffer, 32, "%f", av.double_value());
         out = buffer;
         break;
      case opentelemetry::proto::common::v1::AnyValue::kBoolValue:
         out = av.bool_value() ? "true" : "false";
         break;
      case opentelemetry::proto::common::v1::AnyValue::kBytesValue:
         out = OtlpBytesToHex(av.bytes_value());
         break;
      case opentelemetry::proto::common::v1::AnyValue::kArrayValue:
      case opentelemetry::proto::common::v1::AnyValue::kKvlistValue:
      {
         json_t *json = AnyValueToJson(av);
         char *text = json_dumps(json, JSON_COMPACT | JSON_PRESERVE_ORDER | JSON_REAL_PRECISION(15));
         out = (text != nullptr) ? text : "";
         MemFree(text);
         json_decref(json);
         break;
      }
      default:
         out.clear();
         break;
   }
}

/**
 * Extract resource attributes into a map
 */
static inline void ExtractResourceAttributes(const opentelemetry::proto::resource::v1::Resource& resource, std::map<std::string, std::string>& attributes)
{
   for (int i = 0; i < resource.attributes_size(); i++)
   {
      const auto& kv = resource.attributes(i);
      std::string value;
      AnyValueToString(kv.value(), value);
      nxlog_debug_tag(DEBUG_TAG_OTLP, 7, L"ExtractResourceAttributes: %hs=%hs", kv.key().c_str(), value.c_str());
      attributes.emplace(kv.key(), std::move(value));
   }
}

/**
 * Extract attributes from any protobuf message type that has attributes() returning a repeated KeyValue field.
 */
template<typename T>
static inline void ExtractDataPointAttributes(const T& dp, std::map<std::string, std::string>& attributes)
{
   for (int i = 0; i < dp.attributes_size(); i++)
   {
      const auto& kv = dp.attributes(i);
      std::string value;
      AnyValueToString(kv.value(), value);
      attributes.emplace(kv.key(), std::move(value));
   }
}

#endif
