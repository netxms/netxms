/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.nxmc.modules.datacollection.actions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpValue;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.CreateSnmpTableDciDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.modules.snmp.shared.MibCache;
import org.xnap.commons.i18n.I18n;

/**
 * "Create table DCI from SNMP" action for walk results context menu
 */
public class CreateSnmpTableDciFromWalk extends Action
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSnmpTableDciFromWalk.class);

   private ObjectView view;
   private List<SnmpValue> objects = new ArrayList<SnmpValue>();
   private int lastEntryLen = -1;

   /**
    * Create action.
    *
    * @param view owning view
    */
   public CreateSnmpTableDciFromWalk(ObjectView view)
   {
      super(LocalizationHelper.getI18n(CreateSnmpTableDciFromWalk.class).tr("Create SNMP table DCI..."));
      this.view = view;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      if (objects.isEmpty())
         return;

      List<ColumnDefinition> columns = null;
      String description = "";
      SnmpObjectId selectedOid = objects.get(0).getObjectId();

      // Try to discover all columns from MIB tree for complete column list.
      // This ensures all table columns are found regardless of which walk results are selected.
      MibObject closestMib = MibCache.findObject(selectedOid, false);
      if ((closestMib != null) && (closestMib.getObjectId() != null) && !closestMib.hasChildren())
      {
         // Found column node (leaf) - navigate to entry/table in MIB tree
         MibObject entryNode = closestMib.getParent();
         MibObject tableNode = (entryNode != null) ? entryNode.getParent() : null;
         if ((entryNode != null) && entryNode.hasChildren())
         {
            columns = CreateSnmpTableDci.extractColumnsFromMibTree(entryNode, tableNode);
            lastEntryLen = entryNode.getObjectId().getLength();
            description = (tableNode != null) ? tableNode.getName() : entryNode.getName();
         }
      }

      // Fallback to walk-result-based column discovery
      if ((columns == null) || columns.isEmpty())
      {
         columns = extractColumnsFromWalkResults(objects);
         if (columns.isEmpty())
            return;
         SnmpObjectId entryOidObj = selectedOid.subId(lastEntryLen > 0 ? lastEntryLen : selectedOid.getLength() - 2);
         MibObject mib = MibCache.findObject(entryOidObj, true);
         if (mib != null)
         {
            MibObject tableNode = mib.getParent();
            description = (tableNode != null) ? tableNode.getName() : mib.getName();
         }
         else
         {
            mib = MibCache.findObject(entryOidObj, false);
            description = (mib != null) ? mib.getName() : entryOidObj.toString();
         }
      }

      // DCI name from first selected walk result's column OID
      final String dciName;
      if ((lastEntryLen > 0) && (selectedOid.getLength() > lastEntryLen))
         dciName = selectedOid.subId(lastEntryLen + 1).toString();
      else
         dciName = columns.get(0).getSnmpObjectId().toString();

      final CreateSnmpTableDciDialog dlg = new CreateSnmpTableDciDialog(view.getWindow().getShell(), description, columns, dciName);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      final String finalDescription = dlg.getDescription();
      final int pollingInterval = dlg.getPollingInterval();
      final int retentionTime = dlg.getRetentionTime();
      final List<ColumnDefinition> finalColumns = dlg.getSelectedColumns();
      final long nodeId = objects.get(0).getNodeId();

      new Job(i18n.tr("Creating SNMP table DCI..."), view) {
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create table DCI");
         }

         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            AbstractNode node = (AbstractNode)session.findObjectById(nodeId, AbstractNode.class);
            if (node == null)
               throw new NXCException(RCC.INTERNAL_ERROR);

            DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(node.getObjectId());
            DataCollectionTable table = new DataCollectionTable(dcc, 0);
            table.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
            table.setPollingInterval(Integer.toString(pollingInterval));
            table.setRetentionType(DataCollectionObject.RETENTION_CUSTOM);
            table.setRetentionTime(Integer.toString(retentionTime));
            table.setOrigin(DataOrigin.SNMP);
            table.setStatus(DataCollectionObjectStatus.ACTIVE);
            table.setDescription(finalDescription);
            table.setName(dciName);
            table.setColumns(finalColumns);
            dcc.modifyObject(table);
            dcc.close();
         }
      }.start();
   }

   /**
    * Extract column definitions from SNMP walk results.
    *
    * @param values selected walk result values
    * @return list of column definitions
    */
   private List<ColumnDefinition> extractColumnsFromWalkResults(List<SnmpValue> values)
   {
      // Determine entry OID length using MIB cache for correct instance stripping.
      // Walk result OIDs are structured as entry.column.instance where instance can be multi-component.
      SnmpObjectId firstOid = values.get(0).getObjectId();
      int entryLen = -1;
      MibObject closestMib = MibCache.findObject(firstOid, false);
      if ((closestMib != null) && (closestMib.getObjectId() != null))
      {
         if (!closestMib.hasChildren())
         {
            // Column node (leaf in MIB tree) - entry OID is one component shorter
            entryLen = closestMib.getObjectId().getLength() - 1;
         }
         else
         {
            // Not at column level (entry or table) - assume entry
            entryLen = closestMib.getObjectId().getLength();
         }
      }
      if ((entryLen < 1) || (entryLen >= firstOid.getLength()))
      {
         // Fallback: assume single-component instance
         entryLen = firstOid.getLength() - 2;
      }
      if (entryLen < 1)
         return new ArrayList<ColumnDefinition>();

      lastEntryLen = entryLen;
      SnmpObjectId baseOid = firstOid.subId(entryLen);

      // Collect one representative value per column OID
      Map<Long, SnmpValue> columnMap = new HashMap<Long, SnmpValue>();
      for(SnmpValue v : values)
      {
         SnmpObjectId oid = v.getObjectId();
         if (oid.getLength() > entryLen)
         {
            long columnId = oid.getIdFromPos(entryLen);
            if (!columnMap.containsKey(columnId))
               columnMap.put(columnId, v);
         }
      }

      List<ColumnDefinition> columns = new ArrayList<ColumnDefinition>();
      for(Map.Entry<Long, SnmpValue> entry : columnMap.entrySet())
      {
         SnmpValue v = entry.getValue();
         SnmpObjectId columnOid = new SnmpObjectId(baseOid, entry.getKey());

         String columnName;
         MibObject mib = MibCache.findObject(columnOid, true);
         if (mib != null)
         {
            columnName = mib.getName();
         }
         else
         {
            mib = MibCache.findObject(columnOid, false);
            if (mib != null)
               columnName = mib.getName() + "_" + entry.getKey();
            else
               columnName = Long.toString(entry.getKey());
         }

         ColumnDefinition c = new ColumnDefinition(columnName, columnName);
         c.setDataType(CreateSnmpDci.dciTypeFromAsnType(v.getType()));
         c.setConvertSnmpStringToHex(v.getType() == 0xFFFF);
         c.setSnmpObjectId(columnOid);
         columns.add(c);
      }
      return columns;
   }

   /**
    * Handle selection change in walk results viewer.
    *
    * @param selection new selection
    */
   public void selectionChanged(ISelection selection)
   {
      objects.clear();
      if ((selection instanceof IStructuredSelection) && (((IStructuredSelection)selection).size() > 0))
      {
         for(Object o : ((IStructuredSelection)selection).toList())
         {
            if (o instanceof SnmpValue)
               objects.add((SnmpValue)o);
         }
         setEnabled(objects.size() > 0);
      }
      else
      {
         setEnabled(false);
      }
   }
}
