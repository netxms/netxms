#!/usr/bin/env python3
#
# Generate the built-in ENTSO-E bidding-zone catalog (catalog_data.h) for the
# entsoe subagent.
#
# The catalog maps a human-readable zone name to its ENTSO-E EIC code and the
# list of neighbouring zones (used to build cross-border physical-flow (A11)
# requests). Zone EIC codes and interconnection topology are well-known static
# reference data; this generator embeds an authoritative snapshot so the
# compiled subagent works out of the box, while an external ZoneCatalog= file
# can override/extend it without recompiling.
#
# Source of the embedded snapshot: the EnergieID/entsoe-py library
# (entsoe/mappings.py) Area enum (EIC codes) and NEIGHBOURS dictionary
# (interconnection adjacency). Re-run after updating the snapshot below when
# ENTSO-E bidding-zone topology changes (e.g. new zone splits).
#
# Usage: python3 generate_catalog.py > ../catalog_data.h

RETRIEVED = "2026-07-09 (entsoe-py master)"

# --- EIC codes for the bidding zones we expose (name -> EIC). ---------------
# Only genuine bidding zones are listed; TSO control areas, market-region
# aggregations and border-specific pseudo areas from the source are omitted.
AREA_EIC = {
    "AL": "10YAL-KESH-----5",
    "AT": "10YAT-APG------L",
    "BA": "10YBA-JPCC-----D",
    "BE": "10YBE----------2",
    "BG": "10YCA-BULGARIA-R",
    "BY": "10Y1001A1001A51S",
    "CH": "10YCH-SWISSGRIDZ",
    "CZ": "10YCZ-CEPS-----N",
    "DE_LU": "10Y1001A1001A82H",
    "DK_1": "10YDK-1--------W",
    "DK_2": "10YDK-2--------M",
    "EE": "10Y1001A1001A39I",
    "ES": "10YES-REE------0",
    "FI": "10YFI-1--------U",
    "FR": "10YFR-RTE------C",
    "GB": "10YGB----------A",
    "GR": "10YGR-HTSO-----Y",
    "HR": "10YHR-HEP------M",
    "HU": "10YHU-MAVIR----U",
    "IE_SEM": "10Y1001A1001A59C",
    "IT_BRNN": "10Y1001A1001A699",
    "IT_CALA": "10Y1001C--00096J",
    "IT_CNOR": "10Y1001A1001A70O",
    "IT_CSUD": "10Y1001A1001A71M",
    "IT_FOGN": "10Y1001A1001A72K",
    "IT_NORD": "10Y1001A1001A73I",
    "IT_ROSN": "10Y1001A1001A77A",
    "IT_SARD": "10Y1001A1001A74G",
    "IT_SICI": "10Y1001A1001A75E",
    "IT_SUD": "10Y1001A1001A788",
    "LT": "10YLT-1001A0008Q",
    "LV": "10YLV-1001A00074",
    "ME": "10YCS-CG-TSO---S",
    "MK": "10YMK-MEPSO----8",
    "MT": "10Y1001A1001A93C",
    "NL": "10YNL----------L",
    "NO_1": "10YNO-1--------2",
    "NO_2": "10YNO-2--------T",
    "NO_3": "10YNO-3--------J",
    "NO_4": "10YNO-4--------9",
    "NO_5": "10Y1001A1001A48H",
    "PL": "10YPL-AREA-----S",
    "PT": "10YPT-REN------W",
    "RO": "10YRO-TEL------P",
    "RS": "10YCS-SERBIATSOV",
    "RU": "10Y1001A1001A49F",
    "RU_KGD": "10Y1001A1001A50U",
    "SE_1": "10Y1001A1001A44P",
    "SE_2": "10Y1001A1001A45N",
    "SE_3": "10Y1001A1001A46L",
    "SE_4": "10Y1001A1001A47J",
    "SI": "10YSI-ELES-----O",
    "SK": "10YSK-SEPS-----K",
    "TR": "10YTR-TEIAS----W",
    "UA": "10Y1001C--00003F",
}

# --- Interconnection adjacency (directed as published; symmetrised below). --
# Entries referencing zones outside AREA_EIC (e.g. the historical DE_AT_LU
# combined zone, all-island IE, control areas) are filtered out.
NEIGHBOURS = {
    "BE": ["NL", "FR", "GB", "DE_LU"],
    "NL": ["BE", "DE_LU", "GB", "NO_2", "DK_1"],
    "FR": ["BE", "CH", "DE_LU", "ES", "GB", "IT_NORD"],
    "CH": ["AT", "DE_LU", "FR", "IT_NORD"],
    "AT": ["CH", "CZ", "DE_LU", "HU", "IT_NORD", "SI"],
    "CZ": ["AT", "DE_LU", "PL", "SK"],
    "GB": ["BE", "FR", "IE_SEM", "NL", "NO_2", "DK_1"],
    "NO_2": ["DE_LU", "DK_1", "NL", "NO_1", "NO_5", "GB"],
    "HU": ["AT", "HR", "RO", "RS", "SI", "SK", "UA"],
    "IT_NORD": ["CH", "FR", "SI", "AT", "IT_CNOR"],
    "ES": ["FR", "PT"],
    "SI": ["AT", "HR", "IT_NORD", "HU"],
    "RS": ["AL", "BA", "BG", "HR", "HU", "ME", "MK", "RO"],
    "PL": ["CZ", "DE_LU", "LT", "SE_4", "SK", "UA"],
    "ME": ["AL", "BA", "RS"],
    "DK_1": ["DE_LU", "DK_2", "NO_2", "SE_3", "NL", "GB"],
    "RO": ["BG", "HU", "RS", "UA"],
    "LT": ["BY", "LV", "PL", "RU_KGD", "SE_4"],
    "BG": ["GR", "MK", "RO", "RS", "TR"],
    "SE_3": ["DK_1", "FI", "NO_1", "SE_2", "SE_4"],
    "LV": ["EE", "LT", "RU"],
    "IE_SEM": ["GB"],
    "BA": ["HR", "ME", "RS"],
    "NO_1": ["NO_2", "NO_3", "NO_5", "SE_3"],
    "SE_4": ["DE_LU", "DK_2", "LT", "PL", "SE_3"],
    "NO_5": ["NO_1", "NO_2", "NO_3"],
    "SK": ["CZ", "HU", "PL", "UA"],
    "EE": ["FI", "LV", "RU"],
    "DK_2": ["DE_LU", "DK_1", "SE_4"],
    "FI": ["EE", "NO_4", "RU", "SE_1", "SE_3"],
    "NO_4": ["SE_2", "FI", "NO_3", "SE_1"],
    "SE_1": ["FI", "NO_4", "SE_2"],
    "SE_2": ["NO_3", "NO_4", "SE_1", "SE_3"],
    "DE_LU": ["AT", "BE", "CH", "CZ", "DK_1", "DK_2", "FR", "NO_2", "NL", "PL", "SE_4"],
    "MK": ["BG", "GR", "RS"],
    "PT": ["ES"],
    "GR": ["AL", "BG", "IT_BRNN", "MK", "TR"],
    "NO_3": ["NO_1", "NO_4", "NO_5", "SE_2"],
    "IT_BRNN": ["GR", "IT_SUD"],
    "IT_SUD": ["IT_BRNN", "IT_CSUD", "IT_FOGN", "IT_ROSN", "IT_CALA"],
    "IT_FOGN": ["IT_SUD"],
    "IT_ROSN": ["IT_SICI", "IT_SUD"],
    "IT_CSUD": ["IT_CNOR", "IT_SARD", "IT_SUD"],
    "IT_CNOR": ["IT_NORD", "IT_CSUD", "IT_SARD"],
    "IT_SARD": ["IT_CNOR", "IT_CSUD"],
    "IT_SICI": ["IT_CALA", "IT_ROSN", "MT"],
    "IT_CALA": ["IT_SICI", "IT_SUD"],
    "MT": ["IT_SICI"],
    "HR": ["BA", "HU", "RS", "SI"],
}


def display_name(key):
    return key.replace("_", "-")


def build():
    zones = sorted(AREA_EIC.keys())
    # Symmetric closure restricted to known zones.
    adj = {z: set() for z in zones}
    for z, ns in NEIGHBOURS.items():
        if z not in adj:
            continue
        for n in ns:
            if n in adj:
                adj[z].add(n)
                adj[n].add(z)
    rows = []
    for z in zones:
        neigh = ",".join(display_name(n) for n in sorted(adj[z]))
        rows.append((display_name(z), AREA_EIC[z], neigh))
    return rows


def main():
    rows = build()
    out = []
    out.append("/* Generated by tools/generate_catalog.py - DO NOT EDIT BY HAND. */")
    out.append("/* ENTSO-E bidding-zone catalog snapshot: %s */" % RETRIEVED)
    out.append("")
    out.append("struct BuiltinZone")
    out.append("{")
    out.append("   const char *name;")
    out.append("   const char *eic;")
    out.append("   const char *neighbours;   /* comma-separated zone names */")
    out.append("};")
    out.append("")
    out.append("static const BuiltinZone s_builtinZones[] =")
    out.append("{")
    for name, eic, neigh in rows:
        out.append('   { "%s", "%s", "%s" },' % (name, eic, neigh))
    out.append("};")
    out.append("")
    print("\n".join(out))


if __name__ == "__main__":
    main()
