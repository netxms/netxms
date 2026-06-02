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
 * Convert a single OTLP AnyValue to its string representation.
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
