/**
 * NetXMS - open source network management system
 * Copyright (C) 2023 Raden Solutions
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
package org.netxms.nxmc.modules.assetmanagement.dialogs;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.asset.AssetAttribute;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for selecting asset attributes definition
 */
public class SelectAssetAttributeDlg extends Dialog 
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectAssetAttributeDlg.class);

   private SortableTableViewer viewer;
   private List<AssetAttribute> selectedAttributes;

   /**
    * @param parentShell
    * @param multiSelection
    */
   public SelectAssetAttributeDlg(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Asset Attributes");
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      final String[] names = { i18n.tr("Name"), i18n.tr("Display name") };
      final int[] widths = { 200, 300 };
      viewer = new SortableTableViewer(dialogArea, names, widths, 0, SWT.UP, SWT.FULL_SELECTION | SWT.BORDER | SWT.MULTI);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AttributeListLabelProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            int columnIndex = (Integer)((TableViewer)viewer).getTable().getSortColumn().getData("ID");
            int result = (columnIndex == 0) ?
               ((AssetAttribute)e1).getName().compareToIgnoreCase(((AssetAttribute)e2).getName()) :
               ((AssetAttribute)e1).getEffectiveDisplayName().compareToIgnoreCase(((AssetAttribute)e2).getEffectiveDisplayName());
            return (((TableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.minimumWidth = 300;
      gd.heightHint = 400;
      viewer.getControl().setLayoutData(gd);

      viewer.setInput(Registry.getSession().getAssetManagementSchema().values());
      viewer.packColumns();

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Please select at least one attribute"));
         return;
      }

      selectedAttributes = new ArrayList<AssetAttribute>(selection.size());
      for(Object o : selection.toList())
         selectedAttributes.add((AssetAttribute)o);

      super.okPressed();
   }

   /**
    * Get selection list 
    * 
    * @return
    */
   public List<AssetAttribute> getSelectedAttributes()
   {      
      return selectedAttributes;
   }

   /**
    * Label provider for attribute list
    */
   private static class AttributeListLabelProvider extends LabelProvider implements ITableLabelProvider
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
         AssetAttribute attribute = (AssetAttribute)element;
         return (columnIndex == 0) ? attribute.getName() : attribute.getEffectiveDisplayName();
      }
   }
}
