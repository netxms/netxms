/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.dialogs;

import java.util.List;
import java.util.UUID;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogConstants;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.base.NXCommon;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.EventReferenceType;
import org.netxms.client.events.EventReference;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog to show event references
 */
public class EventReferenceViewDialog extends Dialog
{
   private static final int COLUMN_TYPE = 0;
   private static final int COLUMN_ID = 1;
   private static final int COLUMN_NAME = 2;
   private static final int COLUMN_DESCRIPTION = 3;
   private static final int COLUMN_OWNER_ID = 4;
   private static final int COLUMN_OWNER_NAME = 5;

   private I18n i18n = LocalizationHelper.getI18n(EventReferenceViewDialog.class);
   private NXCSession session = Registry.getSession();
   private List<EventReference> references;
   private SortableTableViewer viewer;
   private String eventName;
   private boolean showWarning;
   private boolean multiChoice;

   /**
    * @param parentShell
    */
   public EventReferenceViewDialog(Shell parentShell, String eventName, List<EventReference> references, boolean showWarning, boolean multiChoice)
   {
      super(parentShell);
      this.references = references;
      this.eventName = eventName;
      this.showWarning = showWarning;
      this.multiChoice = multiChoice;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(showWarning ? "Warning" : String.format("Event References - %s", eventName));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createButtonsForButtonBar(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createButtonsForButtonBar(Composite parent)
   {
      if (showWarning)
      {
         if (multiChoice)
         {
            createButton(parent, IDialogConstants.YES_ID, i18n.tr("&Yes"), true);
            createButton(parent, IDialogConstants.YES_TO_ALL_ID, i18n.tr("Yes To &All"), false);
            createButton(parent, IDialogConstants.NO_ID, i18n.tr("&No"), false);
            createButton(parent, IDialogConstants.NO_TO_ALL_ID, i18n.tr("N&o To All"), false);
         }
         else
         {
            createButton(parent, IDialogConstants.YES_ID, i18n.tr("&Yes"), true);
            createButton(parent, IDialogConstants.NO_ID, i18n.tr("&No"), false);
         }
      }
      else
      {
         createButton(parent, IDialogConstants.CANCEL_ID, i18n.tr("Close"), false);
      }
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#buttonPressed(int)
    */
   @Override
   protected void buttonPressed(int buttonId)
   {
      setReturnCode(buttonId);
      close();
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      if (showWarning)
      {
         Composite warningArea = new Composite(dialogArea, SWT.NONE);
         layout = new GridLayout();
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
         layout.numColumns = 2;
         warningArea.setLayout(layout);

         Label label = new Label(warningArea, SWT.NONE);
         label.setImage(parent.getDisplay().getSystemImage(SWT.ICON_WARNING));

         label = new Label(warningArea, SWT.LEFT);
         label.setText(String.format("Event %s is used in the following entities. Deleting it may cause unexpected system behavior. Are you sure?", eventName));
         label.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, true, false));
      }

      final String[] names = { "Type", "ID", "Name", "Description", "Owner ID", "Owner name" };
      final int[] widths = { 150, 100, 200, 200, 100, 200 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new ReferenceLabelProvider());
      viewer.setComparator(new ReferenceComparator());

      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);

      viewer.setInput(references);
      viewer.packColumns();

      return dialogArea;
   }

   /**
    * Get path for given object.
    *
    * @param objectId object OD
    * @param fallbackName fallback name to be used if path cannot be built
    * @return path for given object or fallback name
    */
   private String getObjectPath(long objectId, String fallbackName)
   {
      AbstractObject object = session.findObjectById(objectId);
      if (object == null)
         return fallbackName;
      return object.getObjectNameWithPath();
   }

   /**
    * Label provider for reference list
    */
   private class ReferenceLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         EventReference r = (EventReference)element;
         switch(columnIndex)
         {
            case COLUMN_TYPE:
               return getTypeName(r.getType());
            case COLUMN_ID:
               return getIdText(r.getId(), r.getGuid());
            case COLUMN_NAME:
               if (r.getType() == EventReferenceType.CONDITION)
                  return getObjectPath(r.getId(), r.getName());
               return r.getName();
            case COLUMN_OWNER_ID:
               return getIdText(r.getOwnerId(), r.getOwnerGuid());
            case COLUMN_OWNER_NAME:
               if ((r.getType() == EventReferenceType.AGENT_POLICY) || (r.getType() == EventReferenceType.DCI))
                  return getObjectPath(r.getOwnerId(), r.getOwnerName());
               return r.getOwnerName();
            case COLUMN_DESCRIPTION:
               return r.getDescription();
         }
         return null;
      }

      /**
       * Get text for ID
       *
       * @param id integer ID
       * @param guid GUID
       * @return text for ID
       */
      private String getIdText(long id, UUID guid)
      {
         if (id != 0)
            return Long.toString(id);
         if (!guid.equals(NXCommon.EMPTY_GUID))
            return guid.toString();
         return null;
      }

      /**
       * Get name for given reference type.
       *
       * @param type reference type
       * @return name for given reference type
       */
      private String getTypeName(EventReferenceType type)
      {
         switch(type)
         {
            case AGENT_POLICY:
               return "Agent Policy";
            case CONDITION:
               return "Condition";
            case DCI:
               return "Data Collection Item";
            case EP_RULE:
               return "Event Processing Policy";
            case SNMP_TRAP:
               return "SNMP Trap";
            case SYSLOG:
               return "Syslog Parser";
            case WINDOWS_EVENT_LOG:
               return "Windows Event Log Parser";
            default:
               return "Error";
         }
      }
   }
   
   /**
    * Comparator for reference list
    */
   private class ReferenceComparator extends ViewerComparator
   {
      /**
       * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
       */
      @Override
      public int compare(Viewer viewer, Object e1, Object e2)
      {
         EventReference r1 = (EventReference)e1;
         EventReference r2 = (EventReference)e2;
         final int column = (Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID"); //$NON-NLS-1$

         String n1, n2;
         int result;
         switch(column)
         {
            case COLUMN_TYPE:
               result = r1.getType().toString().compareTo(r2.getType().toString());
               break;
            case COLUMN_ID:
               result = compareId(r1.getId(), r1.getGuid(), r2.getId(), r2.getGuid());
               break;
            case COLUMN_NAME:
               n1 = (r1.getType() == EventReferenceType.CONDITION) ? getObjectPath(r1.getId(), r1.getName()) : r1.getName();
               n2 = (r2.getType() == EventReferenceType.CONDITION) ? getObjectPath(r2.getId(), r2.getName()) : r2.getName();
               result = n1.compareToIgnoreCase(n2);
               break;
            case COLUMN_OWNER_ID:
               result = compareId(r1.getOwnerId(), r1.getOwnerGuid(), r2.getOwnerId(), r2.getOwnerGuid());
               break;
            case COLUMN_OWNER_NAME:
               n1 = ((r1.getType() == EventReferenceType.AGENT_POLICY) || (r1.getType() == EventReferenceType.DCI)) ? getObjectPath(r1.getOwnerId(), r1.getOwnerName()) : r1.getOwnerName();
               n2 = ((r2.getType() == EventReferenceType.AGENT_POLICY) || (r2.getType() == EventReferenceType.DCI)) ? getObjectPath(r2.getOwnerId(), r2.getOwnerName()) : r2.getOwnerName();
               result = n1.compareToIgnoreCase(n2);
               break;
            case COLUMN_DESCRIPTION:
               result = r1.getDescription().compareToIgnoreCase(r2.getDescription());
               break;
            default:
               result = 0;
               break;
         }

         return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
      }

      /**
       * Compare identifiers.
       *
       * @param id1 first integer ID
       * @param guid1 first GUID
       * @param id2 second integer ID
       * @param guid2 second GUID
       * @return comparison result
       */
      private int compareId(long id1, UUID guid1, long id2, UUID guid2)
      {
         if ((id1 != 0) && (id2 != 0))
            return Long.signum(id1 - id2);
         if ((id1 == 0) && (id2 == 0))
            return guid1.compareTo(guid2);
         return Long.signum(id2 - id1); // GUID based ID is higher than integer ID
      }
   }
}
