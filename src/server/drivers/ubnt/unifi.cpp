/* 
** NetXMS - Network Management System
** Drivers for Ubiquiti Networks devices
** Copyright (C) 2003-2024 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: unifi.cpp
**/

#include "ubnt.h"
#include <nms_core.h>
#include <nms_util.h>
#include <netxms-version.h>
#include <curl/curl.h>
#include <math.h>
#include <map>
#include <assert.h>
#include <stdlib.h>

#define DEBUG_TAG_UBNT _T("ndd.ubnt")

// UTF-8 helper (TCHAR -> std::string)
static std::string toUtf8(const TCHAR *s)
{
#ifdef UNICODE
    char *mb = MBStringFromWideString(s);
    std::string out = (mb != nullptr) ? mb : "";
    MemFree(mb);
    return out;
#else
    return (s != nullptr) ? std::string(s) : std::string();
#endif
}

// UniFi station parsing / matching helpers
namespace UnifiHelpers
{
    /**
     * Parse a single station JSON object into WirelessStationInfo.
     * Returns true if parsing was successful and required fields are present.
     */
    static bool FillStaInfoFromJson(json_t *sta, WirelessStationInfo *info)
    {
        memset(info, 0, sizeof(*info));

        // Skip wired stations
        if (json_t *v = json_object_get(sta, "is_wired"); json_is_true(v))
            return false;

        // MAC (required)
        if (json_t *v = json_object_get(sta, "mac"); json_is_string(v))
        {
            MacAddress mac = MacAddress::parse(json_string_value(v));
            if (!mac.isValid())
                return false;
            memcpy(info->macAddr, mac.value(), MAC_ADDR_LENGTH);
        }
        else
        {
            return false;
        }

        // IP (optional)
        if (json_t *v = json_object_get(sta, "ip"); json_is_string(v))
            info->ipAddr = InetAddress::parse(json_string_value(v));

        // SSID (must have essid or ssid)
        if (json_t *v = json_object_get(sta, "essid"); json_is_string(v))
            mb_to_wchar(json_string_value(v), -1, info->ssid, MAX_SSID_LENGTH);
        else if (json_t *v2 = json_object_get(sta, "ssid"); json_is_string(v2))
            mb_to_wchar(json_string_value(v2), -1, info->ssid, MAX_SSID_LENGTH);
        else
            return false;

        // RSSI (optional)
        if (json_t *v = json_object_get(sta, "rssi"); json_is_integer(v))
            info->rssi = (int)json_integer_value(v);
        else if (json_t *v2 = json_object_get(sta, "signal"); json_is_integer(v2))
            info->rssi = (int)json_integer_value(v2);

        // VLAN (default 1 if not set)
        if (json_t *v = json_object_get(sta, "vlan"); json_is_integer(v))
            info->vlan = (int)json_integer_value(v);
        else
            info->vlan = 1;

        // BSSID (required)
        if (json_t *v = json_object_get(sta, "bssid"); json_is_string(v))
        {
            MacAddress b = MacAddress::parse(json_string_value(v));
            if (!b.isValid())
                return false;
            memcpy(info->bssid, b.value(), MAC_ADDR_LENGTH);
        }
        else
        {
            return false;
        }

        // Radio name (optional)
        if (json_t *v = json_object_get(sta, "radio_name"); json_is_string(v))
            mb_to_wchar(json_string_value(v), -1, info->rfName, MAX_OBJECT_NAME);

        // Match policy
        info->rfIndex = 0;
        info->apMatchPolicy = AP_MATCH_BY_BSSID;

        return true;
    }

    /**
     * Try to match station to one of discovered radios.
     * Must match both BSSID and SSID.
     *
     * Returns true only if station ends up with valid rfIndex and rfName.
     */
    static bool MatchStaToRadio(WirelessStationInfo *sta, const StructArray<RadioInterfaceInfo> *radios)
    {
        if (radios == nullptr)
            return false;

        for (int i = 0; i < radios->size(); i++)
        {
            const RadioInterfaceInfo *r = radios->get(i);
            if ((memcmp(sta->bssid, r->bssid, MAC_ADDR_LENGTH) == 0) && (!_tcsicmp(sta->ssid, r->ssid)))
            {
                sta->rfIndex = r->index;
                _tcslcpy(sta->rfName, r->name, MAX_OBJECT_NAME);

                if (sta->rfIndex != 0 && sta->rfName[0] != 0)
                {
                    nxlog_debug_tag(DEBUG_TAG_UBNT, 6, _T("Matched station MAC=%hs SSID=\"%s\" → radio index=%u name=\"%s\""),
                                    MacAddress(sta->macAddr, MAC_ADDR_LENGTH).toString().cstr(), sta->ssid, sta->rfIndex, sta->rfName);
                    return true;
                }
            }
        }
        return false;
    }

    // Extract station array from UniFi JSON response
    static json_t *GetStationArrayFromJson(const StringBuffer &jsonText)
    {
        // Convert TCHAR JSON (UTF-16 on UNICODE builds) to UTF-8 for jansson
        const std::string jsonUtf8 = toUtf8(jsonText.cstr());

        json_error_t err;
        json_t *root = json_loads(jsonUtf8.c_str(), 0, &err);
        if (root == nullptr)
        {
            nxlog_debug_tag(DEBUG_TAG_UBNT, 3, _T("GetStationArrayFromJson: JSON parse error at line %d: %hs"), err.line, err.text);
            return nullptr;
        }

        json_t *arr = nullptr;
        if (json_is_object(root))
        {
            arr = json_object_get(root, "data"); // borrowed
            if (json_is_array(arr))
                json_incref(arr); // make it owned
        }
        else if (json_is_array(root))
        {
            arr = root;
        }

        if (arr == nullptr)
        {
            json_decref(root);
            return nullptr;
        }

        if (arr != root)
            json_decref(root); // drop original object if we detached array

        return arr; // caller must json_decref()
    }
} // namespace UnifiHelpers

// UniFi SNMP radio discovery helpers
namespace UnifiSnmpHelpers
{
    struct VapInfo
    {
        uint32_t radioIndex = 0;
        std::wstring radioName;
        std::wstring mode;
    };

    struct RadioWalkContext
    {
        StructArray<RadioInterfaceInfo> *radios;
        std::map<std::wstring, uint32_t> ifNameMap;
        std::map<uint32_t, VapInfo> vapInfoMap;
    };

    // Assign SNMP integer into RadioInterfaceInfo field
    template <typename Member>
    static uint32_t AssignIntWalkHandler(SNMP_Variable *v, void *context, Member RadioInterfaceInfo::*field)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_INTEGER)
        {
            const SNMP_ObjectId &oid = v->getName();
            uint32_t idx = oid.getElement(oid.length() - 1);
            if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
            {
                RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
                if (r != nullptr)
                    r->*field = (decltype(r->*field))v->getValueAsInt();
            }
        }
        return SNMP_ERR_SUCCESS;
    }

    // Assign SNMP string into RadioInterfaceInfo TCHAR array field (any size)
    template <size_t N>
    static uint32_t AssignStringWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context, TCHAR (RadioInterfaceInfo::*field)[N], size_t maxLen)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_OCTET_STRING && v->getValueLength() > 0)
        {
            TCHAR buf[MAX_OBJECT_NAME];
            v->getValueAsString(buf, MAX_OBJECT_NAME);

            const SNMP_ObjectId &oid = v->getName();
            uint32_t idx = oid.getElement(oid.length() - 1);

            if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
            {
                RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
                if (r != nullptr)
                    _tcslcpy(r->*field, buf, maxLen);
            }
        }
        return SNMP_ERR_SUCCESS;
    }

    // Assign SNMP string value into VapInfo string-like field (e.g. std::wstring)
    template <typename Member>
    static uint32_t AssignVapStringWalkHandler(SNMP_Variable *v, void *context, Member VapInfo::*field)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_OCTET_STRING && v->getValueLength() > 0)
        {
            TCHAR buf[MAX_OBJECT_NAME];
            v->getValueAsString(buf, MAX_OBJECT_NAME);
            uint32_t vapIndex = v->getName().getElement(v->getName().length() - 1);
            ctx->vapInfoMap[vapIndex].*field = buf;
        }
        return SNMP_ERR_SUCCESS;
    }

    // Assign SNMP integer value into VapInfo integer field
    template <typename Member>
    static uint32_t AssignVapIntWalkHandler(SNMP_Variable *v, void *context, Member VapInfo::*field)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_INTEGER)
        {
            uint32_t vapIndex = v->getName().getElement(v->getName().length() - 1);
            ctx->vapInfoMap[vapIndex].*field = (uint32_t)v->getValueAsUInt();
        }
        return SNMP_ERR_SUCCESS;
    }

    // Name + ifNameMap mapping
    static uint32_t NameWalkHandler(SNMP_Variable *v, SNMP_Transport *t, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);

        uint32_t rc = AssignStringWalkHandler(v, t, ctx, &RadioInterfaceInfo::name, MAX_OBJECT_NAME);
        if (rc != SNMP_ERR_SUCCESS)
            return rc;

        const SNMP_ObjectId &oid = v->getName();
        uint32_t idx = oid.getElement(oid.length() - 1);
        if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
        {
            RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
            if (r != nullptr)
            {
                auto it = ctx->ifNameMap.find(std::wstring(r->name));
                if (it != ctx->ifNameMap.end())
                    r->ifIndex = it->second;

                nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] NameWalkHandler: radio index=%u name=\"%s\" mapped ifIndex=%u"), r->index, r->name, r->ifIndex);
            }
        }
        return SNMP_ERR_SUCCESS;
    }

    // SSID
    static uint32_t SsidWalkHandler(SNMP_Variable *v, SNMP_Transport *t, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        uint32_t rc = AssignStringWalkHandler(v, t, ctx, &RadioInterfaceInfo::ssid, MAX_SSID_LENGTH);
        if (rc != SNMP_ERR_SUCCESS)
            return rc;

        const SNMP_ObjectId &oid = v->getName();
        uint32_t idx = oid.getElement(oid.length() - 1);
        if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
        {
            RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
            if (r != nullptr)
                nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] SsidWalkHandler: radio index=%u SSID=\"%s\""), r->index, r->ssid);
        }

        return SNMP_ERR_SUCCESS;
    }

    // Channel
    static uint32_t ChannelWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_INTEGER)
        {
            const SNMP_ObjectId &oid = v->getName();
            uint32_t idx = oid.getElement(oid.length() - 1);
            if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
            {
                RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
                if (r != nullptr)
                {
                    r->channel = (uint16_t)v->getValueAsUInt();
                    r->band = (r->channel < 15) ? RADIO_BAND_2_4_GHZ : RADIO_BAND_5_GHZ;
                    r->frequency = WirelessChannelToFrequency(r->band, r->channel);
                    nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] ChannelWalkHandler: radio index=%u channel=%u freq=%u band=%s"), r->index, r->channel, r->frequency, RadioBandDisplayName(r->band));
                }
            }
        }
        return SNMP_ERR_SUCCESS;
    }

    // Tx Power
    static uint32_t TxPowerDBmWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if (v->getType() == ASN_INTEGER)
        {
            const SNMP_ObjectId &oid = v->getName();
            uint32_t idx = oid.getElement(oid.length() - 1);
            if ((idx > 0) && (idx <= (uint32_t)ctx->radios->size()))
            {
                RadioInterfaceInfo *r = ctx->radios->get(idx - 1);
                if (r != nullptr)
                {
                    r->powerDBm = v->getValueAsInt();
                    nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] TxPowerDBmWalkHandler: radio index=%u power=%d dBm"), r->index, r->powerDBm);
                }
            }
        }
        return SNMP_ERR_SUCCESS;
    }

    // VAP mode
    static uint32_t VapModeWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        TCHAR buf[64];
        v->getValueAsString(buf, 64);
        uint32_t vapIndex = v->getName().getElement(v->getName().length() - 1);
        ctx->vapInfoMap[vapIndex].mode = buf;
        nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] VapModeWalkHandler: VAP %u mode=\"%s\""), vapIndex, buf);
        return SNMP_ERR_SUCCESS;
    }

    // VAP → radio index
    static uint32_t VapRadioIndexWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        uint32_t radioIndex = v->getValueAsUInt();
        uint32_t vapIndex = v->getName().getElement(v->getName().length() - 1);
        ctx->vapInfoMap[vapIndex].radioIndex = radioIndex;
        nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] VapRadioIndexWalkHandler: VAP %u → radioIndex=%u"), vapIndex, radioIndex);
        return SNMP_ERR_SUCCESS;
    }

    // Radio name
    static uint32_t RadioNameWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        TCHAR buf[MAX_OBJECT_NAME];
        v->getValueAsString(buf, MAX_OBJECT_NAME);
        uint32_t radioIndex = v->getName().getElement(v->getName().length() - 1);
        for (auto &it : ctx->vapInfoMap)
            if (it.second.radioIndex == radioIndex)
                it.second.radioName = buf;
        nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] RadioNameWalkHandler: radioIndex=%u name=\"%s\" assigned to VAPs"), radioIndex, buf);
        return SNMP_ERR_SUCCESS;
    }

    // BSSID
    static uint32_t BssidWalkHandler(SNMP_Variable *v, SNMP_Transport *, void *context)
    {
        auto *ctx = static_cast<RadioWalkContext *>(context);
        if ((v->getType() == ASN_OCTET_STRING) && (v->getValueLength() == MAC_ADDR_LENGTH))
        {
            RadioInterfaceInfo r{};
            r.index = (uint32_t)ctx->radios->size() + 1;
            r.ifIndex = r.index;
            v->getRawValue(r.bssid, MAC_ADDR_LENGTH);
            _tcslcpy(r.name, _T("Radio (SNMP)"), MAX_OBJECT_NAME);
            r.band = RADIO_BAND_UNKNOWN;
            ctx->radios->add(r);
            nxlog_debug_tag(DEBUG_TAG_UBNT, 7, _T("[SNMP] BssidWalkHandler: discovered radio index=%u BSSID=%s"), r.index, MacAddress(r.bssid, MAC_ADDR_LENGTH).toString().cstr());
        }
        return SNMP_ERR_SUCCESS;
    }
} // namespace UnifiSnmpHelpers

void TrimInPlace(StringBuffer &s)
{
    const TCHAR *p = s.cstr();
    size_t len = s.length();
    size_t i = 0;
    while (i < len && _istspace(p[i]))
        i++;
    size_t j = len;
    while (j > i && _istspace(p[j - 1]))
        j--;
    if ((i == 0) && (j == len))
        return; // nothing to trim

    StringBuffer t;
    for (size_t k = i; k < j; k++)
    {
        TCHAR one[2] = {p[k], 0};
        t.append(one);
    }
    s = t;
}

void NormalizeControllerURL(StringBuffer &url)
{
    StringBuffer before(url);
    TrimInPlace(url);
    if ((_tcsnicmp(url.cstr(), _T("http://"), 7) != 0) &&
        (_tcsnicmp(url.cstr(), _T("https://"), 8) != 0))
    {
        StringBuffer t;
        t.append(_T("https://"));
        t.append(url);
        url = t;
    }
    while (url.length() > 0 && url.cstr()[url.length() - 1] == _T('/'))
        url.removeRange(url.length() - 1, 1);

    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("NormalizeControllerURL: [%s] -> [%s]"), before.cstr(), url.cstr());
}

bool ReadCfgStr(const TCHAR *key, StringBuffer &out)
{
    TCHAR buf[1024] = _T("");
    ConfigReadStr(key, buf, 1024, _T(""));
    if (buf[0] != 0)
    {
        out = buf;
        return true;
    }
    return false;
}

bool ReadNodeAttr(NObject *node, const TCHAR *name, StringBuffer &out)
{
    TCHAR buffer[1024];
    if (node->getCustomAttribute(name, buffer, 1024) != nullptr)
    {
        out = buffer;
        return true;
    }
    return false;
}

// Config
struct UnifiConfig
{
    StringBuffer url;
    StringBuffer site;
    StringBuffer token;
    StringBuffer user;
    StringBuffer pass;
    bool verifyTLS = false;
};

// Load config
static bool LoadUnifiConfig(NObject *node, UnifiConfig &cfg)
{
    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("LoadUnifiConfig: reading config"));

    ReadCfgStr(_T("Unifi.ControllerURL"), cfg.url);
    ReadCfgStr(_T("Unifi.Site"), cfg.site);
    ReadCfgStr(_T("Unifi.Token"), cfg.token);
    ReadCfgStr(_T("Unifi.User"), cfg.user);
    ReadCfgStr(_T("Unifi.Password"), cfg.pass);
    {
        StringBuffer v;
        if (ReadCfgStr(_T("Unifi.VerifyTLS"), v) && !v.isEmpty())
            cfg.verifyTLS = (_tcstol(v.cstr(), nullptr, 10) != 0);
    }

    // per-node overrides
    StringBuffer t;
    if (ReadNodeAttr(node, _T("Unifi.ControllerURL"), t) && !t.isEmpty())
        cfg.url = t;
    if (ReadNodeAttr(node, _T("Unifi.Site"), t) && !t.isEmpty())
        cfg.site = t;
    if (ReadNodeAttr(node, _T("Unifi.Token"), t) && !t.isEmpty())
        cfg.token = t;
    if (ReadNodeAttr(node, _T("Unifi.User"), t) && !t.isEmpty())
        cfg.user = t;
    if (ReadNodeAttr(node, _T("Unifi.Password"), t) && !t.isEmpty())
        cfg.pass = t;
    if (ReadNodeAttr(node, _T("Unifi.VerifyTLS"), t) && !t.isEmpty())
        cfg.verifyTLS = (_tcstol(t.cstr(), nullptr, 10) != 0);

    if (cfg.site.isEmpty())
        cfg.site = _T("default");

    NormalizeControllerURL(cfg.url);

    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("LoadUnifiConfig: url=[%s] site=[%s] token=%s verifyTLS=%d"),
                    cfg.url.cstr(), cfg.site.cstr(), cfg.token.isEmpty() ? _T("<empty>") : _T("<set>"), cfg.verifyTLS ? 1 : 0);

    return !cfg.url.isEmpty();
}

// Constructor - Driver identity
UbiquitiUniFiDriver::UbiquitiUniFiDriver()
{
    nxlog_debug_tag(DEBUG_TAG_UBNT, 4, _T("UbiquitiUniFiDriver initialized (version %s)"), NETXMS_VERSION_STRING);
}

/**
 * Get driver name
 */
const TCHAR *UbiquitiUniFiDriver::getName()
{
    return _T("UBNT-UNIFI");
}

/**
 * Get driver version
 */
const TCHAR *UbiquitiUniFiDriver::getVersion()
{
    return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int UbiquitiUniFiDriver::isPotentialDevice(const SNMP_ObjectId &oid)
{
    bool match = oid.startsWith({1, 3, 6, 1, 4, 1, 41112});
    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("UbiquitiUniFiDriver::isPotentialDevice: oid=%s match=%d"), oid.toString().cstr(), match ? 1 : 0);
    return match ? 210 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool UbiquitiUniFiDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId &)
{
    TCHAR model[128];
    uint32_t rc = SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.41112.1.6.3.3.0"), nullptr, 0, model, sizeof(model), SG_STRING_RESULT);
    bool supported = (rc == SNMP_ERR_SUCCESS) && (model[0] != 0);
    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("UbiquitiUniFiDriver::isDeviceSupported: rc=%u model=\"%s\" supported=%d"), rc, (model[0] != 0) ? model : _T("<empty>"), supported ? 1 : 0);
    return supported;
}

/**
 * Returns true if device is a wireless access point. Default implementation always return false.
 * For our purposes it uses the controller for wireless data and we are not concerned otherwise.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a standalone wireless access point
 */
bool UbiquitiUniFiDriver::isWirelessAccessPoint(SNMP_Transport *, NObject *, DriverData *)
{
    return true;
}

/**
 * Get list of radio interfaces for access point. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of radio interfaces for standalone access point
 */
StructArray<RadioInterfaceInfo> *UbiquitiUniFiDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *)
{
    UnifiConfig cfg;
    if (!LoadUnifiConfig(node, cfg))
    {
        nxlog_debug_tag(DEBUG_TAG_UBNT, 4, _T("UbiquitiUniFiDriver::getRadioInterfaces: no controller config, returning nullptr"));
        return nullptr;
    }

    auto radios = new StructArray<RadioInterfaceInfo>(0, 4);
    UnifiSnmpHelpers::RadioWalkContext ctx{radios};

    // ifName -> ifIndex map
    auto buildIfNameMap = [](SNMP_Variable *v, SNMP_Transport *, void *context) -> uint32_t
    {
        auto *map = static_cast<std::map<std::wstring, uint32_t> *>(context);
        TCHAR buf[MAX_OBJECT_NAME];
        v->getValueAsString(buf, MAX_OBJECT_NAME);
        const SNMP_ObjectId &oid = v->getName();
        uint32_t ifIndex = oid.getElement(oid.length() - 1);
        (*map)[buf] = ifIndex;
        return SNMP_ERR_SUCCESS;
    };

    SnmpWalk(snmp, _T(".1.3.6.1.2.1.31.1.1.1.1"), buildIfNameMap, &ctx.ifNameMap);

    auto buildIfDescrMap = [](SNMP_Variable *v, SNMP_Transport *, void *context) -> uint32_t
    {
        auto *map = static_cast<std::map<std::wstring, uint32_t> *>(context);
        TCHAR buf[MAX_OBJECT_NAME];
        v->getValueAsString(buf, MAX_OBJECT_NAME);
        const SNMP_ObjectId &oid = v->getName();
        uint32_t ifIndex = oid.getElement(oid.length() - 1);
        if (map->find(buf) == map->end())
            (*map)[buf] = ifIndex;
        return SNMP_ERR_SUCCESS;
    };

    // Collect VAP details
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.9"), UnifiSnmpHelpers::VapModeWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.1.1.3"), UnifiSnmpHelpers::VapRadioIndexWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.1.1.2"), UnifiSnmpHelpers::RadioNameWalkHandler, &ctx);

    for (auto &it : ctx.vapInfoMap)
        nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("VAP %u → radioIndex=%u, radioName=\"%s\", mode=\"%s\""), it.first, it.second.radioIndex, it.second.radioName.c_str(), it.second.mode.c_str());

    SnmpWalk(snmp, _T(".1.3.6.1.2.1.2.2.1.2"), buildIfDescrMap, &ctx.ifNameMap);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.2"), UnifiSnmpHelpers::BssidWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.7"), UnifiSnmpHelpers::NameWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.6"), UnifiSnmpHelpers::SsidWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.4"), UnifiSnmpHelpers::ChannelWalkHandler, &ctx);
    SnmpWalk(snmp, _T(".1.3.6.1.4.1.41112.1.6.1.2.1.21"), UnifiSnmpHelpers::TxPowerDBmWalkHandler, &ctx);

    nxlog_debug_tag(DEBUG_TAG_UBNT, 5, _T("UbiquitiUniFiDriver::getRadioInterfaces: discovered %d radios"), (int)radios->size());

    if (radios->size() == 0)
    {
        delete radios;
        radios = nullptr;
    }
    return radios;
}

// CURL write buffer
struct CurlGrowBuf
{
    char *data = nullptr;
    size_t size = 0;
    size_t cap = 0;

    ~CurlGrowBuf() { free(data); }

    bool ensure(size_t need)
    {
        if (size + need + 1 <= cap)
            return true;
        size_t ncap = (cap == 0) ? (need + 1024) : cap;
        while (ncap < size + need + 1)
            ncap *= 2;
        char *nd = (char *)realloc(data, ncap);
        if (nd == nullptr)
            return false;
        data = nd;
        cap = ncap;
        return true;
    }

    bool append(const void *src, size_t n)
    {
        if (n == 0)
            return true;
        if (!ensure(n))
            return false;
        memcpy(data + size, src, n);
        size += n;
        data[size] = 0;
        return true;
    }
};

static size_t CurlWriteToGrowBuf(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    if (userdata == nullptr || ptr == nullptr)
        return 0;

    CurlGrowBuf *b = static_cast<CurlGrowBuf *>(userdata);
    const size_t total = size * nmemb;
    if (total == 0)
        return 0;

    if (!b->append(ptr, total))
        return 0;
    return total;
}

static void CurlInitCommon(CURL *curl, bool verifyTLS)
{
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
#ifdef CURLOPT_TCP_KEEPALIVE
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
#endif
    if (!verifyTLS)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
}

// HTTP helpers
static bool HttpGetJson(CURL *curl, const TCHAR *urlW, struct curl_slist *headers, StringBuffer &out, long &code, bool verifyTLS)
{
    CurlGrowBuf body;
    CurlInitCommon(curl, verifyTLS);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteToGrowBuf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    const std::string url = toUtf8(urlW);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    out.clear();
    code = 0;

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
        return false;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

#ifdef UNICODE
    if (body.data != nullptr)
    {
        WCHAR *w = WideStringFromUTF8String(body.data);
        if (w != nullptr)
        {
            out = w;
            MemFree(w);
        }
    }
#else
    if (body.data != nullptr)
        out = body.data;
#endif

    return (code >= 200 && code < 300);
}

static bool HttpPostJson(CURL *curl, const TCHAR *urlW, struct curl_slist *headers, const TCHAR *payloadW, StringBuffer &out, long &code, bool verifyTLS)
{
    CurlGrowBuf body;
    CurlInitCommon(curl, verifyTLS);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteToGrowBuf);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

    const std::string url = toUtf8(urlW);
    const std::string payload = toUtf8(payloadW);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)payload.size());
    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    out.clear();
    code = 0;

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK)
        return false;

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

#ifdef UNICODE
    if (body.data != nullptr)
    {
        WCHAR *w = WideStringFromUTF8String(body.data);
        if (w != nullptr)
        {
            out = w;
            MemFree(w);
        }
    }
#else
    if (body.data != nullptr)
        out = body.data;
#endif

    return (code >= 200 && code < 300);
}

// UniFi controller API
static bool UnifiLogin(CURL *curl, const UnifiConfig &cfg, bool isUnifiOS)
{
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ""); // enable cookie engine

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    StringBuffer url, payload, body;
    long code = 0;

    if (isUnifiOS)
    {
        url.append(cfg.url).append(_T("/api/auth/login"));
        payload.append(_T("{\"username\":\"")).append(cfg.user).append(_T("\",\"password\":\"")).append(cfg.pass).append(_T("\",\"rememberMe\":true}"));
    }
    else
    {
        url.append(cfg.url).append(_T("/api/login"));
        payload.append(_T("{\"username\":\"")).append(cfg.user).append(_T("\",\"password\":\"")).append(cfg.pass).append(_T("\"}"));
    }

    bool ok = HttpPostJson(curl, url.cstr(), headers, payload.cstr(), body, code, cfg.verifyTLS);
    curl_slist_free_all(headers);

    nxlog_debug_tag(DEBUG_TAG_UBNT, 4, _T("UnifiLogin: url=%s user=%s isUnifiOS=%d rc=%d code=%ld"),
                    url.cstr(), cfg.user.cstr(), isUnifiOS ? 1 : 0, ok ? 0 : -1, code);

    return ok && code >= 200 && code < 300;
}

static bool UnifiFetchStations(CURL *curl, const UnifiConfig &cfg, StringBuffer &jsonOut)
{
    struct curl_slist *headers = nullptr;
    if (!cfg.token.isEmpty())
    {
        StringBuffer h;
        h.append(_T("X-API-KEY:")).append(cfg.token);
        const std::string hUtf8 = toUtf8(h.cstr());
        headers = curl_slist_append(headers, hUtf8.c_str());
        headers = curl_slist_append(headers, "Accept: application/json");
    }

    long code = 0;

    // UniFi OS first
    StringBuffer uos;
    uos.append(cfg.url).append(_T("/proxy/network/api/s/")).append(cfg.site).append(_T("/stat/sta"));
    if (HttpGetJson(curl, uos.cstr(), headers, jsonOut, code, cfg.verifyTLS))
    {
        if (headers)
            curl_slist_free_all(headers);
        return true;
    }
    if ((code == 401 || code == 403) && !cfg.user.isEmpty() && !cfg.pass.isEmpty())
    {
        if (UnifiLogin(curl, cfg, true) && HttpGetJson(curl, uos.cstr(), headers, jsonOut, code, cfg.verifyTLS))
        {
            if (headers)
                curl_slist_free_all(headers);
            return true;
        }
    }

    // UniFi Appliance
    StringBuffer legacy;
    legacy.append(cfg.url).append(_T("/api/s/")).append(cfg.site).append(_T("/stat/sta"));
    if (HttpGetJson(curl, legacy.cstr(), headers, jsonOut, code, cfg.verifyTLS))
    {
        if (headers)
            curl_slist_free_all(headers);
        return true;
    }
    if ((code == 401 || code == 403) && !cfg.user.isEmpty() && !cfg.pass.isEmpty())
    {
        if (UnifiLogin(curl, cfg, false) && HttpGetJson(curl, legacy.cstr(), headers, jsonOut, code, cfg.verifyTLS))
        {
            if (headers)
                curl_slist_free_all(headers);
            return true;
        }
    }

    if (headers)
        curl_slist_free_all(headers);
    return false;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *UbiquitiUniFiDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *)
{
    UnifiConfig cfg;
    if (!LoadUnifiConfig(node, cfg))
    {
        nxlog_debug_tag(DEBUG_TAG_UBNT, 4, _T("%s: no controller config"), __func__);
        return nullptr;
    }

    // Radios via SNMP (optional for matching)
    unique_ptr<StructArray<RadioInterfaceInfo>> radios(getRadioInterfaces(snmp, node, nullptr));

    CURL *curl = curl_easy_init();
    if (curl == nullptr)
    {
        nxlog_debug_tag(DEBUG_TAG_UBNT, 3, _T("%s: curl init failed"), __func__);
        return nullptr;
    }

    StringBuffer jsonText;
    ObjectArray<WirelessStationInfo> *stations = nullptr;

    if (UnifiFetchStations(curl, cfg, jsonText))
    {
        json_t *arr = UnifiHelpers::GetStationArrayFromJson(jsonText);
        if (arr != nullptr)
        {
            stations = new ObjectArray<WirelessStationInfo>(0, 32, Ownership::True);

            size_t idx;
            json_t *item;
            json_array_foreach(arr, idx, item)
            {
                if (!json_is_object(item))
                    continue;

                auto *info = new WirelessStationInfo;
                if (UnifiHelpers::FillStaInfoFromJson(item, info) && UnifiHelpers::MatchStaToRadio(info, radios.get())) stations->add(info);
                else
                    delete info;
            }

            if (stations->isEmpty())
            {
                delete stations;
                stations = nullptr;
            }

            json_decref(arr);
        }
    }
    else
    {
        nxlog_debug_tag(DEBUG_TAG_UBNT, 3, _T("%s: failed to fetch stations"), __func__);
    }

    curl_easy_cleanup(curl);
    return stations;
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool UbiquitiUniFiDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *, DriverData *, DeviceHardwareInfo *hwInfo)
{
    TCHAR buf[256];

    if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.41112.1.6.3.3.0"), nullptr, 0, buf, sizeof(buf), SG_STRING_RESULT) == SNMP_ERR_SUCCESS && buf[0] != 0)
    {
        _tcslcpy(hwInfo->productCode, buf, 32);
        _tcslcpy(hwInfo->productName, buf, 128);
    }

    if (SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.4.1.41112.1.6.3.6.0"), nullptr, 0, buf, sizeof(buf), SG_STRING_RESULT) == SNMP_ERR_SUCCESS && buf[0] != 0)
        _tcslcpy(hwInfo->productVersion, buf, 16);

    SNMP_PDU req(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
    req.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.41112.1.6.2.1.1.4.1"))); // agentInventorySerialNumber
    SNMP_PDU *rs = nullptr;
    if (snmp->doRequest(&req, &rs) == SNMP_ERR_SUCCESS)
    {
        const SNMP_Variable *v = rs->getVariable(0);
        if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
        {
            size_t len = v->getValueLength();
            if (len > 0)
            {
                BYTE tmp[32];
                if (len > sizeof(tmp))
                    len = sizeof(tmp);
                v->getRawValue(tmp, len);
                TCHAR s[32];
                BinToStr(tmp, (len > 16) ? 16 : len, s);
                _tcslcpy(hwInfo->serialNumber, s, 32);
            }
        }
        delete rs;
    }

    _tcscpy(hwInfo->vendor, _T("Ubiquiti, Inc."));
    return true;
}
