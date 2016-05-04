/**
 * NetXMS - open source network management system
 * Copyright (C) 2016 RadenSoutions
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
package org.netxms.ui.eclipse.policymanager.views;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart2;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AgentPolicyConfig;
import org.netxms.ui.eclipse.agentmanager.dialogs.SaveStoredConfigDialog;
import org.netxms.ui.eclipse.agentmanager.widgets.AgentConfigEditor;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

public class ConfigPolicyEditor extends ViewPart implements ISaveablePart2
{
   public static final String ID = "org.netxms.ui.eclipse.policymanager.views.ConfigPolicyEditor";
   
   private AgentConfigEditor editor;
   private boolean modified = false;
   private boolean dirty = false;
   private AgentPolicyConfig configPolicy;
   
   private Action actionSave;
   
   @Override
   public void createPartControl(Composite parent)
   {
      parent.setLayout(new FillLayout());
      
      editor = new AgentConfigEditor(parent, SWT.NONE, SWT.H_SCROLL | SWT.V_SCROLL);
      editor.getTextWidget().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            if (!dirty)
            {
               modified = true;
               dirty = true;
               firePropertyChange(PROP_DIRTY);
               actionSave.setEnabled(true);
            }
         }
      });
      
      createActions();
      contributeToActionBars();
      actionSave.setEnabled(false);
   }

   @Override
   public void setFocus()
   {
      editor.setFocus();      
   }
   
   /**
    * Set configuration file content
    * 
    * @param config
    */
   public void setPolicy(final AbstractObject policy)
   {
      configPolicy = (AgentPolicyConfig)policy;
      editor.setText(configPolicy.getFileContent());
      setPartName("Edit Policy \""+ configPolicy.getObjectName() +"\"");
   }
   
   /**
    * Create actions
    */
   private void createActions()
   {      
      actionSave = new Action() {
         @Override
         public void run()
         {
            doSave(null);
            actionSave.setEnabled(false);
            dirty=false;
         }
      };
      actionSave.setText("Save");
      actionSave.setImageDescriptor(SharedIcons.SAVE);
   }
   
   /**
    * Contribute actions to action bar
    */
   private void contributeToActionBars()
   {
      IActionBars bars = getViewSite().getActionBars();
      fillLocalPullDown(bars.getMenuManager());
      fillLocalToolBar(bars.getToolBarManager());
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager
    *           Menu manager for pull-down menu
    */
   private void fillLocalPullDown(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
   }

   /**
    * Fill local tool bar
    * 
    * @param manager
    *           Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      try
      {
         NXCObjectModificationData md = new NXCObjectModificationData(configPolicy.getObjectId());
         md.setConfigFileContent(editor.getText());
         ((NXCSession)ConsoleSharedData.getSession()).modifyObject(md);
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getViewSite().getShell(), "Error updating configuration agent policy object", "Error: " + e.getMessage());
      }
   }

   @Override
   public void doSaveAs()
   {      
   }

   @Override
   public boolean isDirty()
   {
      return dirty || modified;
   }

   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   @Override
   public int promptToSaveOnClose()
   {
      SaveStoredConfigDialog dlg = new SaveStoredConfigDialog(getSite().getShell());
      int rc = dlg.open();
      if (rc == SaveStoredConfigDialog.SAVE_ID)
      {
         modified = false;
         return YES;
      }
      if (rc == SaveStoredConfigDialog.CANCEL)
      {
         return CANCEL;
      }
      return NO;
   }
}
