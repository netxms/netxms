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
package org.netxms.ui.eclipse.topology.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.part.ViewPart;
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.topology.Activator;
import org.netxms.ui.eclipse.topology.Messages;
import org.netxms.ui.eclipse.topology.widgets.SearchResult;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Search results for host connection searches
 *
 */
public class FindByMacAddressView extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.topology.views.FindByMacAddressView"; //$NON-NLS-1$
	private static final String TABLE_CONFIG_PREFIX = "FindByMacAddressView"; //$NON-NLS-1$
	
	private SearchResult serachResultWidget;
   private Button startButton;
   private LabeledText queryEditor; 

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
	@Override
	public void createPartControl(Composite parent)
	{      
      parent.setLayout(new FormLayout());
      
      final Composite searchBar = new Composite(parent, SWT.NONE);
      GridLayout gridLayout = new GridLayout();
      gridLayout.numColumns = 2;
      searchBar.setLayout(gridLayout);
      
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      searchBar.setLayoutData(fd);
      
      queryEditor = new LabeledText(searchBar, SWT.NONE);
      queryEditor.setLabel("Search string");
      queryEditor.getTextControl().addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            if (e.stateMask == 0 && e.keyCode == 13)
            {
               search();
            }
         }
      });
      queryEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      startButton = new Button(searchBar, SWT.PUSH);
      startButton.setImage(SharedIcons.IMG_EXECUTE);
      startButton.setText("Start");
      startButton.setToolTipText("Start search");
      startButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            search();
         }
      });
      startButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));
      
      final Composite mainContent = new Composite(parent, SWT.NONE);
      mainContent.setLayout(new FillLayout());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(searchBar);
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
      mainContent.setLayoutData(fd);
      
      serachResultWidget = new SearchResult(this, mainContent, SWT.NONE, TABLE_CONFIG_PREFIX);
      
      contributeToActionBars();
	}

	/**
	 * Contribute actions to action bars
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		serachResultWidget.fillLocalPullDown(bars.getMenuManager());
		serachResultWidget.fillLocalToolBar(bars.getToolBarManager());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		serachResultWidget.setFocus();
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
		super.dispose();
	}
   
   void search()
   {
      final MacAddress macAddress;
      final String macAddr = queryEditor.getText();
      try
      {
         macAddress = MacAddress.parseMacAddress(macAddr, false);
      }
      catch(MacAddressFormatException e)
      {
         MessageDialogHelper.openError(serachResultWidget.getShell(), Messages.get().EnterMacAddressDlg_Error, Messages.get().EnterMacAddressDlg_IncorrectMacAddress);
         return;
      }
      
      startButton.setEnabled(false);
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob(String.format(Messages.get().FindMacAddress_JobTitle, macAddr), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<ConnectionPoint> cp = session.findConnectionPoints(macAddress.getValue(), 100);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  serachResultWidget.showConnection(cp);
                  startButton.setEnabled(true);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(Messages.get().FindMacAddress_JobError, macAddr);
         }
      }.start();
   }
}
