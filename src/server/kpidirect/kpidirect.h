/*
** NetXMS - Network Management System
** Direct KPI computation provider
** Copyright (C) 2026 Raden Solutions
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
** File: kpidirect.h
**
**/

#ifndef _kpidirect_h_
#define _kpidirect_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxkpi.h>
#include <cmath>

/**
 * This header is shared by the module and by the server-free unit test, so it depends on libnetxms only.
 */

#define DEBUG_TAG_KPIDIRECT   L"kpi.direct"

#define KPIDIRECT_METHOD_ID         L"kpidirect"
#define KPIDIRECT_METHOD_VERSION    L"1.0"
#define KPIDIRECT_DISPLAY_NAME      L"Direct computation"
#define KPIDIRECT_INPUT_MARGIN      3600

/**
 * Method constants. Any change is a new method version.
 */
constexpr int KPIDIRECT_GAP_FACTOR = 4;                  // bridgeable gap in nominal intervals
constexpr double KPIDIRECT_GAP_CAP = 3600.0;             // upper limit on the bridgeable gap, seconds
constexpr double KPIDIRECT_THETA = 1.0;                  // required coverage of a PRIMARY series
constexpr double KPIDIRECT_PLAUSIBILITY_FACTOR = 2.0;    // maximum implied power relative to rated power
constexpr double KPIDIRECT_EPS = 1e-9;                   // tolerance on gap, coverage and plausibility comparisons

/**
 * Result of integrating one series over a day
 */
struct SeriesIntegrationResult
{
   double value;        // canonical units; 0 if refused
   double coverage;     // covered fraction of the day's duration, 0..1
   bool accepted;       // not refused and coverage >= THETA
};

SeriesIntegrationResult IntegrateSeries(const ResolvedSeries& series, time_t dayStart, time_t dayEnd, std::vector<ComputationDiagnostic> *diagnostics);
void ComposeComponents(const ComputationInput& input, ComputationOutput *output);
const wchar_t *DiagnosticCodeName(ComputationDiagnosticCode code);

#endif   /* _kpidirect_h_ */
