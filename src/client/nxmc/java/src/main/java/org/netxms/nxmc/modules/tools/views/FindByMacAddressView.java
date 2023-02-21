/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.nxmc.modules.tools.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
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
import org.netxms.base.MacAddress;
import org.netxms.base.MacAddressFormatException;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.tools.widgets.SearchResult;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Search results for MAC address search
 */
public class FindByMacAddressView extends View
{
   private final static I18n i18n = LocalizationHelper.getI18n(FindByMacAddressView.class);
   
	private static final String TABLE_CONFIG_PREFIX = "FindByMacAddressView";

	private SearchResult serachResultWidget;
   private Button startButton;
   private LabeledText queryEditor; 

   /**
    * Create find by MAC address view
    */
   public FindByMacAddressView()
   {
      super(i18n.tr("MAC Address Search"), ResourceManager.getImageDescriptor("icons/tool-views/search_history.png"), "FindByMacAddress", false);
   }

   /**
    * Post clone action
    */
   protected void postClone(View origin)
   {    
      super.postClone(origin);
      FindByMacAddressView view = (FindByMacAddressView)origin;
      queryEditor.setText(view.queryEditor.getText());
      serachResultWidget.copyResults(view.serachResultWidget);
   }   

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
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
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      serachResultWidget.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      serachResultWidget.fillLocalPullDown(manager);
   }

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
      if (!serachResultWidget.isDisposed())
	      queryEditor.setFocus();
	}
   
   /**
    * Search for MAC address
    */
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
         MessageDialogHelper.openError(serachResultWidget.getShell(), i18n.tr("Error"), i18n.tr("MAC address entered is incorrect. Please enter correct MAC address."));
         return;
      }

      startButton.setEnabled(false);
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Searching for MAC address %s in the network"), macAddr), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return String.format(i18n.tr("Search for MAC address %s failed"), macAddr);
         }
      }.start();
   }
}
