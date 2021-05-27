/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.datacollection.views;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.text.templates.Template;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DataCollectionWidget;
import org.netxms.nxmc.modules.datacollection.widgets.LastValuesWidget;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionCommon;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.VisibilityValidator;
import org.xnap.commons.i18n.I18n;

/**
 * "Last Values" view
 */
public class LastValuesView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(LastValuesView.class);

   private DataCollectionCommon dataView = null;
   private Action actionEnableSaveMode;
   private boolean editModeEnabled;
   private Composite parent;

   /**
    * @param name
    * @param image
    */
   public LastValuesView()
   {
      super(i18n.tr("Data Collection"), ResourceManager.getImageDescriptor("icons/object-views/last_values.png"), "DataCollection", true); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      this.parent = parent;
      if (editModeEnabled)
      {
         dataView = new DataCollectionWidget(this, parent, SWT.NONE, getObject(), "LastValuesView", new VisibilityValidator() { //$NON-NLS-1$
            @Override
            public boolean isVisible()
            {
               return LastValuesView.this.isActive();
            }
         });    
      }
      else 
      {
         dataView = new LastValuesWidget(this, parent, SWT.NONE, getObject(), "LastValuesView", new VisibilityValidator() { //$NON-NLS-1$
            @Override
            public boolean isVisible()
            {
               return LastValuesView.this.isActive();
            }
         });    
      }
      setViewerAndFilter(dataView.getViewer(), dataView.getFilter()); 
      createAction();
   }
   
   private void createView()
   {
      if (dataView != null)
         dataView.dispose();

      if (editModeEnabled)
      {
         dataView = new DataCollectionWidget(this, parent, SWT.NONE, getObject(), "LastValuesView", new VisibilityValidator() { //$NON-NLS-1$
            @Override
            public boolean isVisible()
            {
               return LastValuesView.this.isActive();
            }
         });    
      }
      else 
      {
         dataView = new LastValuesWidget(this, parent, SWT.NONE, getObject(), "LastValuesView", new VisibilityValidator() { //$NON-NLS-1$
            @Override
            public boolean isVisible()
            {
               return LastValuesView.this.isActive();
            }
         });    
      }
      setViewerAndFilter(dataView.getViewer(), dataView.getFilter()); 

      dataView.postContentCreate(); 
      dataView.layout();    
      parent.layout();       
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {  
      dataView.postContentCreate();
   }
   

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if (context == null)
         return false;
      
      if (context instanceof DataCollectionTarget)
         return true;
      
      if (context instanceof Template)
      {
         editModeEnabled = true;
         return true;
      }
      
      return false;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      dataView.setDataCollectionTarget(object);
         if (isActive())
            dataView.refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      dataView.refresh(); 
         
   }
   
   /**
    * Create mode change action
    */
   private void createAction()
   {
      actionEnableSaveMode = new Action(i18n.tr("Edit mode"), SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editModeEnabled = actionEnableSaveMode.isChecked();
            createView();
         }
      }; 
      actionEnableSaveMode.setChecked(editModeEnabled); 
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      super.fillLocalToolbar(manager);
      manager.add(actionEnableSaveMode);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 30;
   }
}
