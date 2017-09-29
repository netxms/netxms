/* 
** NetXMS subagent for AIX
** Copyright (C) 2004-2016 Victor Kirhenshtein
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
**/

#include "aix_subagent.h"
#include <lvm.h>
#include <sys/cfgodm.h>
#include <odmi.h>

/**
 * Convert unique_id structure to text representation
 */
static char *UniqueIdToText(struct unique_id *uid, char *buffer)
{
   sprintf(buffer, "%08x%08x%08x%08x", uid->word1, uid->word2, uid->word3, uid->word4);
   return buffer;
}

/**
 * Parse text representation of unique_id structure
 */
static bool ParseUniqueId(const char *text, struct unique_id *uid)
{
   memset(uid, 0, sizeof(struct unique_id));
   if (strlen(text) != 32)
      return false;

   char temp[16], *eptr;
   memcpy(temp, text, 8);
   temp[8] = 0;
   uid->word1 = strtoul(temp, &eptr, 16);
   if (*eptr != 0)
      return false;

   memcpy(temp, &text[8], 8);
   uid->word2 = strtoul(temp, &eptr, 16);
   if (*eptr != 0)
      return false;

   memcpy(temp, &text[16], 8);
   uid->word3 = strtoul(temp, &eptr, 16);
   if (*eptr != 0)
      return false;

   memcpy(temp, &text[24], 8);
   uid->word4 = strtoul(temp, &eptr, 16);
   return *eptr == 0;
}


/**
 * Physical volume information
 */
class PhysicalVolume
{
private:
   char m_name[64];
   struct unique_id m_id;
   UINT64 m_bytesTotal;
   UINT64 m_bytesUsed;
   UINT64 m_ppSize;
   UINT64 m_ppTotal;
   UINT64 m_ppUsed;
   UINT64 m_ppStale;
   UINT64 m_ppResync;
   long m_state;
   char m_statusText[32];

public:
   PhysicalVolume(struct querypv *pv, struct unique_id *id)
   {
      // read volume name
      char query[128], buffer[32];
      sprintf(query, "attribute='pvid' and value='%s'", UniqueIdToText(id, buffer));
      struct CuAt object;
      long rc = CAST_FROM_POINTER(odm_get_first(CuAt_CLASS, query, &object), long);
      if ((rc != 0) && (rc != -1))
      {
         strlcpy(m_name, object.name, 64);
         m_name[63] = 0;
      }
      else
         {
         UniqueIdToText(id, m_name);
      }
      
      memcpy(&m_id, id, sizeof(unique_id));
      m_ppSize = _ULL(1) << pv->ppsize;
      m_ppTotal = pv->pp_count;
      m_ppUsed = pv->alloc_ppcount;
      m_bytesTotal = m_ppTotal * m_ppSize;
      m_bytesUsed = m_ppUsed * m_ppSize;
      
      m_ppStale = 0;
      m_ppResync = 0;
      for(int i = 0; i < (int)pv->pp_count; i++)
      {
         if (pv->pp_map[i].pp_state & LVM_PPSTALE)
            m_ppStale++;
         if (pv->pp_map[i].pp_state & LVM_PPRESYNC)
            m_ppResync++;
      }
      
      m_state = pv->pv_state;
      if (m_state & LVM_PVACTIVE)
         strcpy(m_statusText, "active");
      else if (m_state & LVM_PVMISSING)
         strcpy(m_statusText, "missing");
      else if (m_state & LVM_PVREMOVED)
         strcpy(m_statusText, "removed");
      else
         strcpy(m_statusText, "undefined");
   }
   
   const char *getName() { return m_name; }
   UINT64 getPhyPartTotal() { return m_ppTotal; }
   UINT64 getPhyPartFree() { return m_ppTotal - m_ppUsed; }
   UINT64 getPhyPartUsed() { return m_ppUsed; }
   UINT64 getPhyPartStale() { return m_ppStale; }
   UINT64 getPhyPartResync() { return m_ppResync; }
   UINT64 getBytesTotal() { return m_bytesTotal; }
   UINT64 getBytesFree() { return m_bytesTotal - m_bytesUsed; }
   UINT64 getBytesUsed() { return m_bytesUsed; }
   long getState() { return m_state; }
   const char *getStatusText() { return m_statusText; }
};

/**
 * Logical volume information
 */
class LogicalVolume
{
private:
   char m_name[LVM_NAMESIZ];
   struct lv_id m_id;
   UINT64 m_size;
   bool m_open;
   long m_state;
   char m_statusText[64];
   
public:
   LogicalVolume(struct querylv *lv, struct lv_id *id)
   {
      strcpy(m_name, lv->lvname);
      memcpy(&m_id, id, sizeof(struct lv_id));
      m_size = (UINT64)lv->currentsize * (_ULL(1) << lv->ppsize);
      m_state = lv->lv_state;
      m_open = (lv->open_close == LVM_QLVOPEN);
      
      const char *st;
      switch(m_state)
      {
         case LVM_LVDEFINED:
            st = "syncd";
            break;
         case LVM_LVSTALE:
            st = "stale";
            break;
         case LVM_LVMIRBKP:
            st = "mirror";
            break;
         case LVM_PASSIVE_RECOV:
            st = "recovery";
            break;
         default:
            st = "unknown";
            break;
      }
      sprintf(m_statusText, "%s/%s", m_open ? "open" : "closed", st);
   }
   
   const char *getName() { return m_name; }
   bool isOpen() { return m_open; }
   long getState() { return m_state; }
   const char *getStatusText() { return m_statusText; }
   UINT64 getSize() { return m_size; }
};

/**
 * Volume group information
 */
class VolumeGroup
{
private:
   char m_name[LVM_NAMESIZ];
   struct unique_id m_id;
   UINT64 m_ppSize;
   UINT64 m_ppTotal;
   UINT64 m_ppResync;
   UINT64 m_ppStale;
   UINT64 m_ppFree;
   UINT64 m_bytesTotal;
   UINT64 m_bytesFree;
   ObjectArray<LogicalVolume> *m_logicalVolumes;
   ObjectArray<PhysicalVolume> *m_physicalVolumes;

   VolumeGroup(const char *name, struct unique_id *id)
   {
      strlcpy(m_name, name, LVM_NAMESIZ);
      m_name[LVM_NAMESIZ - 1] = 0;
      memcpy(&m_id, id, sizeof(struct unique_id));
      m_logicalVolumes = new ObjectArray<LogicalVolume>(16, 16, true);
      m_physicalVolumes = new ObjectArray<PhysicalVolume>(16, 16, true);
   }

public:
   ~VolumeGroup()
   {
      delete m_logicalVolumes;
      delete m_physicalVolumes;
   }
   
   const char *getName() { return m_name; }
   UINT64 getPhyPartTotal() { return m_ppTotal; }
   UINT64 getPhyPartFree() { return m_ppFree; }
   UINT64 getPhyPartUsed() { return m_ppTotal - m_ppFree; }
   UINT64 getPhyPartStale() { return m_ppStale; }
   UINT64 getPhyPartResync() { return m_ppResync; }
   UINT64 getBytesTotal() { return m_bytesTotal; }
   UINT64 getBytesFree() { return m_bytesFree; }
   UINT64 getBytesUsed() { return m_bytesTotal - m_bytesFree; }
   int getActivePhysicalVolumes();
   
   ObjectArray<LogicalVolume> *getLogicalVolumes() { return m_logicalVolumes; }
   LogicalVolume *getLogicalVolume(const char *name);
   
   ObjectArray<PhysicalVolume> *getPhysicalVolumes() { return m_physicalVolumes; }
   PhysicalVolume *getPhysicalVolume(const char *name);
   
   static VolumeGroup *create(const char *name, const char *id);
};

/**
 * Create volume group object
 */
VolumeGroup *VolumeGroup::create(const char *name, const char *id)
{
   struct unique_id uid;
   if (!ParseUniqueId(id, &uid))
   {
      nxlog_debug(5, _T("AIX: VolumeGroup::create: cannot parse ID '%hs'"), id);
      return NULL;
   }
   
   struct queryvg *vgdata;
   int rc = lvm_queryvg(&uid, &vgdata, NULL);
   if (rc != 0)
   {
      nxlog_debug(5, _T("AIX: VolumeGroup::create: lvm_queryvg failed for VG %hs (%hs)"), name, id);
      return NULL;
   }
   
   VolumeGroup *vg = new VolumeGroup(name, &uid);
   vg->m_ppSize = _ULL(1) << vgdata->ppsize;
   vg->m_ppFree = vgdata->freespace;
   vg->m_bytesFree = vg->m_ppFree * vg->m_ppSize;
   
   // Query physical volumes
   vg->m_ppTotal = 0;
   vg->m_ppStale = 0;
   vg->m_ppResync = 0;
   for(int i = 0; i < (int)vgdata->num_pvs; i++)
   {
      struct querypv *pv;
      rc = lvm_querypv(&uid, &vgdata->pvs[i].pv_id, &pv, NULL);
      if (rc != 0)
      {
         nxlog_debug(5, _T("AIX: VolumeGroup::create: lvm_querypv failed for VG %hs (%hs) PV %d"), name, id, i);
         goto failure;
      }
      
      PhysicalVolume *pvobj = new PhysicalVolume(pv, &vgdata->pvs[i].pv_id);
      vg->m_physicalVolumes->add(pvobj);
      vg->m_ppTotal += pv->pp_count;
      vg->m_ppStale += pvobj->getPhyPartStale();
      vg->m_ppResync += pvobj->getPhyPartResync();
      
      free(pv->pp_map);
      free(pv);
   }
   vg->m_bytesTotal = vg->m_ppTotal * vg->m_ppSize;
   
   // Query logical volumes
   for(int i = 0; i < (int)vgdata->num_lvs; i++)
   {
      struct querylv *lv;
      rc = lvm_querylv(&vgdata->lvs[i].lv_id, &lv, NULL);
      if (rc != 0)
      {
         nxlog_debug(5, _T("AIX: VolumeGroup::create: lvm_querylv failed for VG %hs (%hs) LV %hs"), name, id, vgdata->lvs[i].lvname);
         goto failure;
      }
      
      vg->m_logicalVolumes->add(new LogicalVolume(lv, &vgdata->lvs[i].lv_id));
      
      for(int j = 0; j < LVM_NUMCOPIES; j++)
         free(lv->mirrors[j]);
      free(lv);
   }
   
   free(vgdata->pvs);
   free(vgdata->lvs);
   free(vgdata);
   return vg;
   
failure:
   free(vgdata->pvs);
   free(vgdata->lvs);
   free(vgdata);
   delete vg;
   return NULL;
}

/**
 * Get logical volume by name
 */
LogicalVolume *VolumeGroup::getLogicalVolume(const char *name)
{
   for(int i = 0; i < m_logicalVolumes->size(); i++)
   {
      LogicalVolume *lv = m_logicalVolumes->get(i);
      if (!strcmp(lv->getName(), name))
         return lv;
   }
   return NULL;
}

/**
 * Get physical volume by name
 */
PhysicalVolume *VolumeGroup::getPhysicalVolume(const char *name)
{
   for(int i = 0; i < m_physicalVolumes->size(); i++)
   {
      PhysicalVolume *pv = m_physicalVolumes->get(i);
      if (!strcmp(pv->getName(), name))
         return pv;
   }
   return NULL;
}

/**
 * Get number of ative physical volumes
 */
int VolumeGroup::getActivePhysicalVolumes()
{
   int count = 0;
   for(int i = 0; i < m_physicalVolumes->size(); i++)
   {
      if (m_physicalVolumes->get(i)->getState() & LVM_PVACTIVE)
         count++;
   }
   return count;
}

/**
 * Read all volume groups
 */
static ObjectArray<VolumeGroup> *ReadVolumeGroups()
{
   ObjectArray<VolumeGroup> *vgs = new ObjectArray<VolumeGroup>(16, 16, true);
   struct CuAt object;
   long rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, "attribute='vgserial_id'", &object, ODM_FIRST), long);
   while((rc != 0) && (rc != -1))
   {
      VolumeGroup *vg = VolumeGroup::create(object.name, object.value);
      if (vg != NULL)
         vgs->add(vg);
      rc = CAST_FROM_POINTER(odm_get_obj(CuAt_CLASS, NULL, &object, ODM_NEXT), long);
   }
   return vgs;
}

/**
 * LVM data cache
 */
static Mutex s_lvmAccessLock;
static ObjectArray<VolumeGroup> *s_volumeGroups = NULL;
static time_t s_timestamp = 0;

/**
 * Acquire LVM data
 */
static ObjectArray<VolumeGroup> *AcquireLvmData()
{
   s_lvmAccessLock.lock();
   time_t now = time(NULL);
   if ((now - s_timestamp > 60) || (s_volumeGroups == NULL))
   {
      delete s_volumeGroups;
      s_volumeGroups = ReadVolumeGroups();
      s_timestamp = now;
   }
   return s_volumeGroups;
}

/**
 * Release LVM data
 */
inline void ReleaseLvmData()
{
   s_lvmAccessLock.unlock();
}

/**
 * Clear LVM data
 */
void ClearLvmData()
{
   s_lvmAccessLock.lock();
   delete_and_null(s_volumeGroups);
   s_lvmAccessLock.unlock();
}

/**
 * Acquire volume group
 */
VolumeGroup *AcquireVolumeGroup(const char *name)
{
   ObjectArray<VolumeGroup> *vgs = AcquireLvmData();
   for(int i = 0; i < vgs->size(); i++)
   {
      VolumeGroup *vg = vgs->get(i);
      if (!strcmp(vg->getName(), name))
         return vg;
   }
   ReleaseLvmData();
   return NULL;
}

/**
 * Handler for LVM.VolumeGroups list
 */
LONG H_LvmVolumeGroups(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ObjectArray<VolumeGroup> *vgs = AcquireLvmData();
   for(int i = 0; i < vgs->size(); i++)
      value->addMBString(vgs->get(i)->getName());
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for LVM.VolumeGroups table
 */
LONG H_LvmVolumeGroupsTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("STATUS"), DCI_DT_STRING, _T("Status"));
   value->addColumn(_T("LVOL_TOTAL"), DCI_DT_UINT, _T("Logical volumes"));
   value->addColumn(_T("PVOL_TOTAL"), DCI_DT_UINT, _T("Physical volumes"));
   value->addColumn(_T("PVOL_ACTIVE"), DCI_DT_UINT, _T("Active physical volumes"));
   value->addColumn(_T("SIZE"), DCI_DT_UINT64, _T("Size"));
   value->addColumn(_T("PARTITIONS"), DCI_DT_UINT64, _T("Partitions"));
   value->addColumn(_T("USED"), DCI_DT_UINT64, _T("Used"));
   value->addColumn(_T("USED_PP"), DCI_DT_UINT64, _T("Used partitions"));
   value->addColumn(_T("USED_PCT"), DCI_DT_UINT64, _T("Used %"));
   value->addColumn(_T("FREE"), DCI_DT_UINT64, _T("Free"));
   value->addColumn(_T("FREE_PP"), DCI_DT_UINT64, _T("Free partitions"));
   value->addColumn(_T("FREE_PCT"), DCI_DT_UINT64, _T("Free %"));
   value->addColumn(_T("STALE_PP"), DCI_DT_UINT64, _T("Stale PP"));
   value->addColumn(_T("RESYNC_PP"), DCI_DT_UINT64, _T("Resync PP"));
   
   ObjectArray<VolumeGroup> *vgs = AcquireLvmData();
   for(int i = 0; i < vgs->size(); i++)
   {
      value->addRow();
      VolumeGroup *vg = vgs->get(i);
#ifdef UNICODE
      value->setPreallocated(0, WideStringFromMBString(vg->getName()));
#else
      value->set(0, vg->getName());
#endif
      value->set(1, _T("online"));
      value->set(2, vg->getLogicalVolumes()->size());
      value->set(3, vg->getPhysicalVolumes()->size());
      value->set(4, vg->getActivePhysicalVolumes());
      value->set(5, vg->getBytesTotal());
      value->set(6, vg->getPhyPartTotal());
      value->set(7, vg->getBytesUsed());
      value->set(8, vg->getPhyPartUsed());
      value->set(9, vg->getPhyPartUsed() * 100.0 / vg->getPhyPartTotal());
      value->set(10, vg->getBytesFree());
      value->set(11, vg->getPhyPartFree());
      value->set(12, vg->getPhyPartFree() * 100.0 / vg->getPhyPartTotal());
      value->set(13, vg->getPhyPartStale());
      value->set(14, vg->getPhyPartResync());
   }
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for volume group parameters
 */
LONG H_LvmVolumeGroupInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   LONG rc = SYSINFO_RC_SUCCESS;
   switch(CAST_FROM_POINTER(arg, int))
   {
      case LVM_VG_FREE:
         ret_uint64(value, vg->getBytesFree());
         break;
      case LVM_VG_FREE_PERC:
         ret_double(value, vg->getPhyPartFree() * 100.0 / vg->getPhyPartTotal());
         break;
      case LVM_VG_LVOL_TOTAL:
         ret_int(value, vg->getLogicalVolumes()->size());
         break;
      case LVM_VG_PVOL_ACTIVE:
         ret_int(value, vg->getActivePhysicalVolumes());
         break;
      case LVM_VG_PVOL_TOTAL:
         ret_int(value, vg->getPhysicalVolumes()->size());
         break;
      case LVM_VG_RESYNC:
         ret_uint64(value, vg->getPhyPartResync());
         break;
      case LVM_VG_STALE:
         ret_uint64(value, vg->getPhyPartStale());
         break;
      case LVM_VG_TOTAL:
         ret_uint64(value, vg->getBytesTotal());
         break;
      case LVM_VG_USED:
         ret_uint64(value, vg->getBytesUsed());
         break;
      case LVM_VG_USED_PERC:
         ret_double(value, vg->getPhyPartUsed() * 100.0 / vg->getPhyPartTotal());
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   ReleaseLvmData();
   return rc;
}

/**
 * Handler for LVM.LogicalVolumes(*) list
 */
LONG H_LvmLogicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   ObjectArray<LogicalVolume> *lvs = vg->getLogicalVolumes();      
   for(int i = 0; i < lvs->size(); i++)
      value->addMBString(lvs->get(i)->getName());
   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for LVM.LogicalVolumes list
 */
LONG H_LvmAllLogicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ObjectArray<VolumeGroup> *vgs = AcquireLvmData();
   for(int i = 0; i < vgs->size(); i++)
   {
      char name[LVM_NAMESIZ * 2];
      VolumeGroup *vg = vgs->get(i);
      strcpy(name, vg->getName());
      strcat(name, "/");
      int npos = strlen(name);
      ObjectArray<LogicalVolume> *lvs = vg->getLogicalVolumes();      
      for(int j = 0; j < lvs->size(); j++)
      {
         strcpy(&name[npos], lvs->get(j)->getName());
         value->addMBString(name);
      }
   }   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for LVM.LogicalVolumes(*) table
 */
LONG H_LvmLogicalVolumesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("STATUS"), DCI_DT_STRING, _T("Status"));
   value->addColumn(_T("SIZE"), DCI_DT_UINT64, _T("Size"));
      
   ObjectArray<LogicalVolume> *lvs = vg->getLogicalVolumes();      
   for(int i = 0; i < lvs->size(); i++)
   {
      value->addRow();
      LogicalVolume *lv = lvs->get(i);
#ifdef UNICODE
      value->setPreallocated(0, WideStringFromMBString(lv->getName()));
      value->setPreallocated(1, WideStringFromMBString(lv->getStatusText()));
#else
      value->set(0, lv->getName());
      value->set(1, lv->getStatusText());
#endif
      value->set(2, lv->getSize());
   }
   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for logical volume parameters
 */
LONG H_LvmLogicalVolumeInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ * 2], lvName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ * 2) ||
       !AgentGetParameterArgA(param, 2, lvName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   // check if volume name given in form vg/lv
   if (lvName[0] == 0)
   {
      char *s = strchr(vgName, '/');
      if (s != NULL)
      {
         *s = 0;
         s++;
         strcpy(lvName, s);
      }
      else
      {
         return SYSINFO_RC_NO_SUCH_INSTANCE;
      }
   }
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   LogicalVolume *lv = vg->getLogicalVolume(lvName);
   if (lv == NULL)
   {
      ReleaseLvmData();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }
   
   LONG rc = SYSINFO_RC_SUCCESS;
   switch(CAST_FROM_POINTER(arg, int))
   {
      case LVM_LV_SIZE:
         ret_uint64(value, lv->getSize());
         break;
      case LVM_LV_STATUS:
         ret_mbstring(value, lv->getStatusText());
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   ReleaseLvmData();
   return rc;
}

/**
 * Handler for LVM.PhysicalVolumes(*) list
 */
LONG H_LvmPhysicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   ObjectArray<PhysicalVolume> *pvs = vg->getPhysicalVolumes();  
   for(int i = 0; i < pvs->size(); i++)
      value->addMBString(pvs->get(i)->getName());
   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for LVM.PhysicalVolumes list
 */
LONG H_LvmAllPhysicalVolumes(const TCHAR *param, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   ObjectArray<VolumeGroup> *vgs = AcquireLvmData();
   for(int i = 0; i < vgs->size(); i++)
   {
      char name[LVM_NAMESIZ * 2];
      VolumeGroup *vg = vgs->get(i);
      strcpy(name, vg->getName());
      strcat(name, "/");
      int npos = strlen(name);
      ObjectArray<PhysicalVolume> *pvs = vg->getPhysicalVolumes();   
      for(int j = 0; j < pvs->size(); j++)
      {
         strcpy(&name[npos], pvs->get(j)->getName());
         value->addMBString(name);
      }
   }   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for LVM.PhysicalVolumes(*) table
 */
LONG H_LvmPhysicalVolumesTable(const TCHAR *param, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("STATUS"), DCI_DT_STRING, _T("Status"));
   value->addColumn(_T("SIZE"), DCI_DT_UINT64, _T("Size"));
   value->addColumn(_T("PARTITIONS"), DCI_DT_UINT64, _T("Partitions"));
   value->addColumn(_T("USED"), DCI_DT_UINT64, _T("Used"));
   value->addColumn(_T("USED_PP"), DCI_DT_UINT64, _T("Used partitions"));
   value->addColumn(_T("USED_PCT"), DCI_DT_UINT64, _T("Used %"));
   value->addColumn(_T("FREE"), DCI_DT_UINT64, _T("Free"));
   value->addColumn(_T("FREE_PP"), DCI_DT_UINT64, _T("Free partitions"));
   value->addColumn(_T("FREE_PCT"), DCI_DT_UINT64, _T("Free %"));
   value->addColumn(_T("STALE_PP"), DCI_DT_UINT64, _T("Stale PP"));
   value->addColumn(_T("RESYNC_PP"), DCI_DT_UINT64, _T("Resync PP"));
      
   ObjectArray<PhysicalVolume> *pvs = vg->getPhysicalVolumes();  
   for(int i = 0; i < pvs->size(); i++)
   {
      value->addRow();
      PhysicalVolume *pv = pvs->get(i);
#ifdef UNICODE
      value->setPreallocated(0, WideStringFromMBString(pv->getName()));
      value->setPreallocated(1, WideStringFromMBString(pv->getStatusText()));
#else
      value->set(0, pv->getName());
      value->set(1, pv->getStatusText());
#endif
      value->set(2, pv->getBytesTotal());
      value->set(3, pv->getPhyPartTotal());
      value->set(4, pv->getBytesUsed());
      value->set(5, pv->getPhyPartUsed());
      value->set(6, pv->getPhyPartUsed() * 100.0 / vg->getPhyPartTotal());
      value->set(7, pv->getBytesFree());
      value->set(8, pv->getPhyPartFree());
      value->set(9, pv->getPhyPartFree() * 100.0 / vg->getPhyPartTotal());
      value->set(10, pv->getPhyPartStale());
      value->set(11, pv->getPhyPartResync());
   }
   
   ReleaseLvmData();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Handler for physical volume parameters
 */
LONG H_LvmPhysicalVolumeInfo(const TCHAR *param, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   char vgName[LVM_NAMESIZ * 2], pvName[LVM_NAMESIZ];
   if (!AgentGetParameterArgA(param, 1, vgName, LVM_NAMESIZ * 2) ||
       !AgentGetParameterArgA(param, 2, pvName, LVM_NAMESIZ))
      return SYSINFO_RC_UNSUPPORTED;
      
   // check if volume name given in form vg/pv
   if (pvName[0] == 0)
   {
      char *s = strchr(vgName, '/');
      if (s != NULL)
      {
         *s = 0;
         s++;
         strcpy(pvName, s);
      }
      else
      {
         return SYSINFO_RC_NO_SUCH_INSTANCE;
      }
   }
      
   VolumeGroup *vg = AcquireVolumeGroup(vgName);
   if (vg == NULL)
      return SYSINFO_RC_NO_SUCH_INSTANCE;
      
   PhysicalVolume *pv = vg->getPhysicalVolume(pvName);
   if (pv == NULL)
   {
      ReleaseLvmData();
      return SYSINFO_RC_NO_SUCH_INSTANCE;
   }
   
   LONG rc = SYSINFO_RC_SUCCESS;
   switch(CAST_FROM_POINTER(arg, int))
   {
      case LVM_PV_FREE:
         ret_uint64(value, pv->getBytesFree());
         break;
      case LVM_PV_FREE_PERC:
         ret_double(value, pv->getPhyPartFree() * 100.0 / pv->getPhyPartTotal());
         break;
      case LVM_PV_RESYNC:
         ret_uint64(value, pv->getPhyPartResync());
         break;
      case LVM_PV_STALE:
         ret_uint64(value, pv->getPhyPartStale());
         break;
      case LVM_PV_STATUS:
         ret_mbstring(value, pv->getStatusText());
         break;
      case LVM_PV_TOTAL:
         ret_uint64(value, pv->getBytesTotal());
         break;
      case LVM_PV_USED:
         ret_uint64(value, pv->getBytesUsed());
         break;
      case LVM_PV_USED_PERC:
         ret_double(value, pv->getPhyPartUsed() * 100.0 / pv->getPhyPartTotal());
         break;
      default:
         rc = SYSINFO_RC_UNSUPPORTED;
         break;
   }
   ReleaseLvmData();
   return rc;
}
