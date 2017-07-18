/*
** NetXMS XEN hypervisor subagent
** Copyright (C) 2017 Raden Solutions
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
** File: vm.cpp
**
**/

#include "xen.h"

/**
 * Resolve domain name to ID
 */
LONG XenResolveDomainName(const char *name, uint32_t *domId)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);
   int rc = (libxl_name_to_domid(ctx, name, domId) == 0) ? SYSINFO_RC_SUCCESS : SYSINFO_RC_NO_SUCH_INSTANCE;
   libxl_ctx_free(ctx);
   return rc;
}

/**
 * Handler for XEN.VirtualMachines list
 */
LONG H_XenDomainList(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);

   LONG rc = SYSINFO_RC_ERROR;

   int count = 0;
   libxl_dominfo *domains = libxl_list_domain(ctx, &count);
   if (domains != NULL)
   {
      for(int i = 0; i < count; i++)
      {
         value->addMBString(libxl_domid_to_name(ctx, domains[i].domid));
      }
      libxl_dominfo_list_free(domains, count);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nxlog_debug(4, _T("XEN: call to libxl_list_domain failed"));
   }

   libxl_ctx_free(ctx);
   return rc;
}

/**
 * Handler for XEN.VirtualMachines table
 */
LONG H_XenDomainTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   libxl_ctx *ctx;
   XEN_CONNECT(ctx);

   LONG rc = SYSINFO_RC_ERROR;

   int count = 0;
   libxl_dominfo *domains = libxl_list_domain(ctx, &count);
   if (domains != NULL)
   {
      value->addColumn(_T("ID"), DCI_DT_INT, _T("ID"), true);
      value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"));
      value->addColumn(_T("STATE"), DCI_DT_STRING, _T("State"));
      value->addColumn(_T("MEM_CURRENT"), DCI_DT_UINT64, _T("Current memory"));
      value->addColumn(_T("MEM_SHARED"), DCI_DT_UINT64, _T("Shared memory"));
      value->addColumn(_T("MEM_PAGED"), DCI_DT_UINT64, _T("Paged memory"));
      value->addColumn(_T("MEM_OUTSTANDING"), DCI_DT_UINT64, _T("Outstanding memory"));
      value->addColumn(_T("MEM_MAX"), DCI_DT_UINT64, _T("Max memory"));
      value->addColumn(_T("CPU_COUNT"), DCI_DT_INT, _T("CPU count"));
      value->addColumn(_T("CPU_TIME"), DCI_DT_UINT64, _T("CPU time"));

      for(int i = 0; i < count; i++)
      {
         value->addRow();

         libxl_dominfo *d = &domains[i];
         value->set(0, d->domid);
         value->set(1, libxl_domid_to_name(ctx, d->domid));

         TCHAR state[6] = _T("-----");
         if (d->running)
            state[0] = _T('R');
         if (d->blocked)
            state[1] = _T('B');
         if (d->paused)
            state[2] = _T('P');
         if (d->shutdown)
            state[3] = _T('S');
         if (d->dying)
            state[4] = _T('D');
         value->set(2, state);

         value->set(3, d->current_memkb * _LL(1024));
         value->set(4, d->shared_memkb * _LL(1024));
         value->set(5, d->paged_memkb * _LL(1024));
         value->set(6, d->outstanding_memkb * _LL(1024));
         value->set(7, d->max_memkb * _LL(1024));

         value->set(8, d->vcpu_online);
         value->set(9, (UINT64)d->cpu_time);
      }
      libxl_dominfo_list_free(domains, count);
      rc = SYSINFO_RC_SUCCESS;
   }
   else
   {
      nxlog_debug(4, _T("XEN: call to libxl_list_domain failed"));
   }

   libxl_ctx_free(ctx);
   return rc;
}
