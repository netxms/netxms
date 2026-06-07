/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.dialogs;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.CheckStateChangedEvent;
import org.eclipse.jface.viewers.CheckboxTableViewer;
import org.eclipse.jface.viewers.ICheckStateListener;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Dialog for choosing which OTLP metric attribute keys form the instance key.
 */
public class SelectOtlpAttributeKeysDlg extends Dialog
{
   private final I18n i18n = LocalizationHelper.getI18n(SelectOtlpAttributeKeysDlg.class);

   private List<String> keys;
   private Set<String> initialSelection;
   private CheckboxTableViewer viewer;
   private List<String> selectedKeys = new ArrayList<String>();

   /**
    * @param parentShell parent shell
    * @param metricName name of the metric whose attribute keys are offered
    * @param keys available attribute keys
    * @param initialSelection keys that should be pre-checked (can be null)
    */
   public SelectOtlpAttributeKeysDlg(Shell parentShell, String metricName, List<String> keys, Collection<String> initialSelection)
   {
      super(parentShell);
      setShellStyle(getShellStyle() | SWT.RESIZE);
      this.keys = keys;
      this.initialSelection = new HashSet<String>();
      if (initialSelection != null)
         this.initialSelection.addAll(initialSelection);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Instance Key Attributes"));
      newShell.setSize(400, 350);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);

      Label label = new Label(dialogArea, SWT.WRAP);
      label.setText(i18n.tr("Select the attribute keys that uniquely identify an instance. The metric name is always part of the key."));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 360;
      label.setLayoutData(gd);

      viewer = CheckboxTableViewer.newCheckList(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.V_SCROLL);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      viewer.getTable().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setInput(keys.toArray());
      for(String key : keys)
      {
         if (initialSelection.contains(key))
            viewer.setChecked(key, true);
      }
      viewer.addCheckStateListener(new ICheckStateListener() {
         @Override
         public void checkStateChanged(CheckStateChangedEvent event)
         {
         }
      });

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      selectedKeys.clear();
      // Preserve the order in which keys were presented
      Set<Object> checked = new HashSet<Object>();
      for(Object o : viewer.getCheckedElements())
         checked.add(o);
      for(String key : keys)
      {
         if (checked.contains(key))
            selectedKeys.add(key);
      }
      super.okPressed();
   }

   /**
    * @return comma-separated list of selected attribute keys, preserving presentation order
    */
   public String getSelectedKeysAsString()
   {
      return String.join(",", selectedKeys);
   }
}
