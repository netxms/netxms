/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.ui.eclipse.snmp.propertypages;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Zone;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * "Agent Shared Secrets" property page for zone objects
 */
public class ZoneAgentSecretConfiguration extends PropertyPage
{
   private Zone zone;
   private List<String> agentSecrets;
   private TableViewer secretList;
   private Button buttonMoveUp;
   private Button buttonMoveDown;
   private Button buttonAdd;
   private Button buttonDelete;
   private boolean modified = false;
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{
		zone = (Zone)getElement().getAdapter(Zone.class);
		noDefaultAndApplyButton();

		Composite dialogArea = new Composite(parent, SWT.NONE);	
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
		dialogArea.setLayout(layout);
      
      createAgentSecretSection(dialogArea);
      
      loadConfig();
		return dialogArea;
	}

	/**
	 * Create agent secret section
	 * 
	 * @param dialogArea
	 */
   private void createAgentSecretSection(Composite dialogArea)
	{      
      Composite clientArea = new Composite(dialogArea, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      GridData gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      clientArea.setLayoutData(gridData);
      
      secretList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      gridData = new GridData();
      gridData.verticalAlignment = GridData.FILL;
      gridData.grabExcessVerticalSpace = true;
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      gridData.horizontalSpan = 2;
      gridData.heightHint = 150;
      secretList.getTable().setLayoutData(gridData);
      secretList.setContentProvider(new ArrayContentProvider());
      
      Composite buttonsLeft = new Composite(clientArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsLeft.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.LEFT;
      buttonsLeft.setLayoutData(gridData);
      
      buttonMoveUp = new Button(buttonsLeft, SWT.PUSH);
      buttonMoveUp.setText("&Up");
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonMoveUp.setLayoutData(rd);
      buttonMoveUp.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSharedSecret(true);
         }
      });
      
      buttonMoveDown = new Button(buttonsLeft, SWT.PUSH);
      buttonMoveDown.setText("&Down");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonMoveDown.setLayoutData(rd);
      buttonMoveDown.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            moveSharedSecret(false);
         }
      });
      
      Composite buttonsRight = new Composite(clientArea, SWT.NONE);
      buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttonLayout.marginRight = 0;
      buttonsRight.setLayout(buttonLayout);
      gridData = new GridData();
      gridData.horizontalAlignment = SWT.RIGHT;
      buttonsRight.setLayoutData(gridData);

      buttonAdd = new Button(buttonsRight, SWT.PUSH);
      buttonAdd.setText("Add");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);
      buttonAdd.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            addSharedSecret();
         }
      });
      
      buttonDelete = new Button(buttonsRight, SWT.PUSH);
      buttonDelete.setText("Delete");
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonDelete.setLayoutData(rd);
      buttonDelete.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            removeSharedSecret();
         }
      });
      
      secretList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)secretList.getSelection();
            buttonMoveUp.setEnabled(selection.size() == 1);
            buttonMoveDown.setEnabled(selection.size() == 1);
            buttonDelete.setEnabled(selection.size() > 0);
         }
      });
	}
	
   /**
    * Load configured secrets from server
    */
	private void loadConfig()
	{
      final NXCSession session = ConsoleSharedData.getSession();
	   ConsoleJob job = new ConsoleJob(Messages.get().NetworkCredentials_LoadingConfig, null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            agentSecrets = session.getAgentSecrets(zone.getUIN());
            
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  secretList.setInput(agentSecrets);     
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().NetworkCredentials_ErrorLoadingConfig;
         }
      };
      job.setUser(false);
      job.start();
	}
	
   /**
    * Add new secret
    */
   private void addSharedSecret()
   {
      InputDialog dlg = new InputDialog(getShell(), "Add shared secret", 
            "Please enter shared secret", "", null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         agentSecrets.add(value);
         secretList.setInput(agentSecrets);
         modified = true;
      }
   }

   protected void removeSharedSecret()
   {
      IStructuredSelection selection = (IStructuredSelection)secretList.getSelection();
      if (!selection.isEmpty())
      {
         for(Object o : selection.toList())
         {
            agentSecrets.remove(o);
         }
         secretList.setInput(agentSecrets.toArray());
         modified = true;
      }
   }

   /**
    * Move agent shared secret up or down
    * 
    * @param up true to move up, false to move down
    */
   private void moveSharedSecret(boolean up)
   {
      IStructuredSelection selection = (IStructuredSelection)secretList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = agentSecrets.indexOf(o);
            if (up)
            {
               if (index >= 1)
                  Collections.swap(agentSecrets, index - 1, index);
            }
            else
            {
               if ((index + 1) != agentSecrets.size())
                  Collections.swap(agentSecrets, index + 1, index);
            }
         }
         secretList.setInput(agentSecrets);
         modified = true;
      }
   }
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
   private boolean applyChanges(final boolean isApply)
	{
      if (!modified)
         return true;     // Nothing to apply
      
		if (isApply)
			setValid(false);
		
		
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(String.format(Messages.get().NetworkCredentials_SaveConfig, zone.getObjectName()), null, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            session.updateSharedSecrets(zone.getUIN(), agentSecrets);   
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get().NetworkCredentials_ErrorSavingConfig;
			}
		}.start();
		return true;
	}
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
	}
}
