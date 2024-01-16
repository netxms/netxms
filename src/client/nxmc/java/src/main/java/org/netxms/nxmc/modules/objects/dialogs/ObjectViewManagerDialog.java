/**
 * NetXMS - open source network management system
 * Copyright (C) 2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.dialogs;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.jface.viewers.ICheckStateProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.FilterText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for managing visibility of object views.
 */
public class ObjectViewManagerDialog extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(ObjectViewManagerDialog.class);

   private List<View> objectViews;
   private PreferenceStore preferenceStore = PreferenceStore.getInstance();
   private String filterString = null;
   private Map<String, Boolean> changedElements = new HashMap<>();

   /**
    * Create view manager dialog.
    *
    * @param parentShell parent shell
    * @param objectViews list of object views
    */
   public ObjectViewManagerDialog(Shell parentShell, List<View> objectViews)
   {
      super(parentShell);
      this.objectViews = objectViews;
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Object View Manager"));
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);

      new Label(dialogArea, SWT.NONE).setText(i18n.tr("Views that are not marked in the list below will not be displayed in object context"));

      Composite listGroup = new Composite(dialogArea, SWT.BORDER);
      layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      listGroup.setLayout(layout);
      listGroup.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      FilterText filter = new FilterText(listGroup, SWT.NONE, null, false, true);
      filter.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      CheckboxTableViewer viewer = CheckboxTableViewer.newCheckList(listGroup, SWT.CHECK);
      GridData gd = new GridData(SWT.FILL, SWT.FILL, true, true);
      gd.heightHint = 400;
      gd.widthHint = 270;
      viewer.getControl().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((View)element).getName();
         }
      });
      viewer.addFilter(new ViewerFilter() {
         @Override
         public boolean select(Viewer viewer, Object parentElement, Object element)
         {
            return (filterString == null) || filterString.isEmpty() || ((View)element).getName().toLowerCase().contains(filterString);
         }
      });
      viewer.setCheckStateProvider(new ICheckStateProvider() {
         @Override
         public boolean isGrayed(Object element)
         {
            return false;
         }

         @Override
         public boolean isChecked(Object element)
         {
            return !preferenceStore.getAsBoolean("HideView." + ((View)element).getBaseId(), false);
         }
      });
      viewer.addCheckStateListener(new ICheckStateListener() {
         @Override
         public void checkStateChanged(CheckStateChangedEvent event)
         {
            String id = ((View)event.getElement()).getBaseId();
            changedElements.put(id, !event.getChecked());
         }
      });
      viewer.setInput(objectViews);

      filter.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            filterString = filter.getText();
            viewer.refresh();
         }
      });

      dialogArea.layout(true, true); // Fix layout issue on Mac OS X

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      for(Entry<String, Boolean> e : changedElements.entrySet())
      {
         preferenceStore.set("HideView." + e.getKey(), e.getValue());
      }
      super.okPressed();
   }
}
