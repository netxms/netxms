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
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.window.Window;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.DataCollectionObjectStatus;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.snmp.MibObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.dialogs.CreateSnmpTableDciDialog;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.xnap.commons.i18n.I18n;

/**
 * "Create table DCI from SNMP" action for MIB tree context menu
 */
public class CreateSnmpTableDci extends Action
{
   private I18n i18n = LocalizationHelper.getI18n(CreateSnmpTableDci.class);

   private ObjectView view;
   private MibObject mibObject;

   /**
    * Create action.
    *
    * @param view owning view
    */
   public CreateSnmpTableDci(ObjectView view)
   {
      super(LocalizationHelper.getI18n(CreateSnmpTableDci.class).tr("Create SNMP table DCI..."));
      this.view = view;
   }

   /**
    * Check if given MIB object is a table column (leaf node whose grandparent is a SEQUENCE).
    *
    * @param object MIB object to check
    * @return true if object represents a table column node
    */
   public static boolean isTableColumn(MibObject object)
   {
      if (object == null)
         return false;
      MibObject parent = object.getParent();
      if (parent == null)
         return false;
      MibObject grandparent = parent.getParent();
      return (grandparent != null) && (grandparent.getType() == MibObject.MIB_TYPE_SEQUENCE);
   }

   /**
    * Set MIB object for tree-based creation.
    *
    * @param object selected MIB object
    */
   public void setMibObject(MibObject object)
   {
      this.mibObject = object;
      setEnabled(true);
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      if (mibObject == null)
         return;

      // mibObject is always a column node (parent = entry, grandparent = table)
      MibObject entryNode = mibObject.getParent();
      MibObject tableNode = entryNode.getParent();
      String description = (tableNode != null) ? tableNode.getName() : entryNode.getName();
      List<ColumnDefinition> columns = extractColumnsFromMibTree(entryNode, tableNode);
      if (columns.isEmpty())
         return;
      final String dciName = mibObject.getObjectId().toString();
      mibObject = null;

      final CreateSnmpTableDciDialog dlg = new CreateSnmpTableDciDialog(view.getWindow().getShell(), description, columns, dciName);
      if (dlg.open() != Window.OK)
         return;

      final NXCSession session = Registry.getSession();
      final String finalDescription = dlg.getDescription();
      final int pollingInterval = dlg.getPollingInterval();
      final int retentionTime = dlg.getRetentionTime();
      final List<ColumnDefinition> finalColumns = dlg.getSelectedColumns();
      final long nodeId = view.getObjectId();

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
    * Extract column definitions from MIB tree.
    *
    * @param entryNode the entry (SEQID) node whose children are columns
    * @param tableNode the table (SEQUENCE) node with index information (may be null)
    * @return list of column definitions
    */
   static List<ColumnDefinition> extractColumnsFromMibTree(MibObject entryNode, MibObject tableNode)
   {
      // Check index on both table (SEQUENCE) and entry (SEQID) nodes - different MIBs store it in different places
      Set<String> indexColumns = new HashSet<String>();
      String indexStr = (tableNode != null) ? tableNode.getIndex() : "";
      if (indexStr.isEmpty())
         indexStr = entryNode.getIndex();
      if (!indexStr.isEmpty())
      {
         for(String s : indexStr.split(","))
         {
            String trimmed = s.trim();
            if (!trimmed.isEmpty())
               indexColumns.add(trimmed);
         }
      }

      List<ColumnDefinition> columns = new ArrayList<ColumnDefinition>();
      for(MibObject child : entryNode.getChildObjects())
      {
         ColumnDefinition c = new ColumnDefinition(child.getName(), child.getName());
         c.setDataType(dciTypeFromMibType(child.getType()));
         c.setSnmpObjectId(child.getObjectId());
         if (indexColumns.contains(child.getName()))
            c.setInstanceColumn(true);
         columns.add(c);
      }
      return columns;
   }

   /**
    * Convert MIB type constant to DCI data type.
    *
    * @param mibType MIB_TYPE_* constant
    * @return corresponding DCI data type
    */
   static DataType dciTypeFromMibType(int mibType)
   {
      switch(mibType)
      {
         case MibObject.MIB_TYPE_INTEGER:
         case MibObject.MIB_TYPE_INTEGER32:
            return DataType.INT32;
         case MibObject.MIB_TYPE_INTEGER64:
            return DataType.INT64;
         case MibObject.MIB_TYPE_UNSIGNED32:
         case MibObject.MIB_TYPE_GAUGE:
         case MibObject.MIB_TYPE_GAUGE32:
         case MibObject.MIB_TYPE_TIMETICKS:
         case MibObject.MIB_TYPE_UINTEGER:
            return DataType.UINT32;
         case MibObject.MIB_TYPE_COUNTER:
         case MibObject.MIB_TYPE_COUNTER32:
            return DataType.COUNTER32;
         case MibObject.MIB_TYPE_COUNTER64:
            return DataType.COUNTER64;
         default:
            return DataType.STRING;
      }
   }
}
