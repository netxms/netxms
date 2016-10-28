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
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.AgentPolicyLogParser;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.serverconfig.widgets.LogParserEditor;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserModifyListener;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

public class LogParserPolicyEditor  extends ViewPart implements ISaveablePart
{
   public static final String ID = "org.netxms.ui.eclipse.policymanager.views.LogParserPolicyEditor";
   
   private AgentPolicyLogParser logParser;

   private NXCSession session;
   private LogParserEditor editor;
   private boolean modified = false;
   private String content;
   private Action actionRefresh;
   private Action actionSave;
   
   /* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = (NXCSession)ConsoleSharedData.getSession();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
   {
      editor = new LogParserEditor(parent, SWT.NONE, false);
      editor.addModifyListener(new LogParserModifyListener() {
         @Override
         public void modifyParser()
         {
            setModified(true);
         }
      });
      
      createActions();
      contributeToActionBars();
      
      refresh();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionRefresh = new RefreshAction(this) {
         @Override
         public void run()
         {
            refresh();
         }
      };
      
      actionSave = new Action("Save", SharedIcons.SAVE) {
         @Override
         public void run()
         {
            save();
         }
      };
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
      manager.add(actionRefresh);
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
      manager.add(new Separator());
      manager.add(actionRefresh);
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
   @Override
   public void setFocus()
   {
      editor.setFocus();
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
   @Override
   public void doSave(IProgressMonitor monitor)
   {
      editor.getDisplay().syncExec(new Runnable() {
         @Override
         public void run()
         {
            content = editor.getParserXml();
         }
      });
      try
      {
         session.setServerConfigClob("SyslogParser", content); //$NON-NLS-1$
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getSite().getShell(), "Error saving log parser", 
               String.format("Error saving log parser: %s", e.getLocalizedMessage()));
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
   @Override
   public void doSaveAs()
   {
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
   @Override
   public boolean isDirty()
   {
      return modified;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
   @Override
   public boolean isSaveAsAllowed()
   {
      return false;
   }

   /* (non-Javadoc)
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
   @Override
   public boolean isSaveOnCloseNeeded()
   {
      return modified;
   }

   /**
    * Refresh viewer
    */
   private void refresh()
   {
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Confirm Refresh", "This will destroy all unsaved changes. Are you sure? "))
            return;
      }
      
      actionSave.setEnabled(false);
      if(logParser != null)
         editor.setParserXml(logParser.getFileContent());
      setModified(false);
   }
   
   /**
    * Save config
    */
   private void save()
   {
      final String xml = editor.getParserXml();
      actionSave.setEnabled(false);
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Save log parser configuration", this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            NXCObjectModificationData md = new NXCObjectModificationData(logParser.getObjectId());
            md.setConfigFileContent(xml);
            session.modifyObject(md);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  setModified(false);
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Error while saving log parser configuration";
         }
      }.start();
   }
   
   /**
    * Mark view as modified
    */
   private void setModified(boolean b)
   {
      if (b != modified)
      {
         modified = b;
         firePropertyChange(PROP_DIRTY);
         actionSave.setEnabled(modified);
      }
   }

   public void setPolicy(AbstractObject policy)
   {
      logParser = (AgentPolicyLogParser)policy;
      editor.setParserXml(logParser.getFileContent());    
      setPartName("Edit Policy \""+ logParser.getObjectName() +"\"");
   }
   
}
