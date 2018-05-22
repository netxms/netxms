/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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
package org.netxms.ui.eclipse.objectbrowser.preferencepages;

import org.eclipse.jface.preference.BooleanFieldEditor;
import org.eclipse.jface.preference.FieldEditorPreferencePage;
import org.eclipse.jface.preference.StringFieldEditor;
import org.eclipse.jface.util.PropertyChangeEvent;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.netxms.ui.eclipse.objectbrowser.Activator;

/**
 * Preferences page for object browser
 */
public class ObjectBrowser extends FieldEditorPreferencePage implements IWorkbenchPreferencePage
{
   private BooleanFieldEditor useServerFilter;
   private Composite autoApplyParent;
   private Composite delayParent;
   private Composite minLengthParent;
   private BooleanFieldEditor autoApply;
   private StringFieldEditor delay;
   private StringFieldEditor minLength;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
    */
   @Override
   public void init(IWorkbench workbench)
   {
      setPreferenceStore(Activator.getDefault().getPreferenceStore());
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#createFieldEditors()
    */
   @Override
   protected void createFieldEditors()
   {
      addField(new BooleanFieldEditor("ObjectBrowser.showStatusIndicator", "Show &status indicator", getFieldEditorParent())); //$NON-NLS-1$
      addField(new BooleanFieldEditor("ObjectBrowser.showFilter", "Show &filter", getFieldEditorParent())); //$NON-NLS-1$
      
      useServerFilter = new BooleanFieldEditor("ObjectBrowser.useServerFilterSettings", "&Use server settings for object filter", getFieldEditorParent()); //$NON-NLS-1$
      addField(useServerFilter);
      
      autoApplyParent = getFieldEditorParent();
      autoApply = new BooleanFieldEditor("ObjectBrowser.filterAutoApply", "&Apply filter automatically", autoApplyParent); //$NON-NLS-1$
      addField(autoApply);
      
      delayParent = getFieldEditorParent();
      delay = new StringFieldEditor("ObjectBrowser.filterDelay", "Filter &delay", delayParent); //$NON-NLS-1$
      addField(delay);
      delay.setEmptyStringAllowed(false);
      delay.setTextLimit(5);
      
      minLengthParent = getFieldEditorParent();
      minLength = new StringFieldEditor("ObjectBrowser.filterMinLength", "Filter &minimal length", minLengthParent); //$NON-NLS-1$
      addField(minLength);
      minLength.setEmptyStringAllowed(false);
      minLength.setTextLimit(3);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#initialize()
    */
   @Override
   protected void initialize()
   {
      super.initialize();
      boolean enabled = !useServerFilter.getBooleanValue();
      autoApply.setEnabled(enabled, autoApplyParent);
      delay.setEnabled(enabled, delayParent);
      minLength.setEnabled(enabled, minLengthParent);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.FieldEditorPreferencePage#propertyChange(org.eclipse.jface.util.PropertyChangeEvent)
    */
   @Override
   public void propertyChange(PropertyChangeEvent event)
   {
      super.propertyChange(event);
      boolean enabled = !useServerFilter.getBooleanValue();
      autoApply.setEnabled(enabled, autoApplyParent);
      delay.setEnabled(enabled, delayParent);
      minLength.setEnabled(enabled, minLengthParent);
   }
}
