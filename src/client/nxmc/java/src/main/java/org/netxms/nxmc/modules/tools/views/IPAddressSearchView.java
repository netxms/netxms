/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.topology.ConnectionPoint;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.modules.tools.widgets.SearchResult;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * IP address search view
 */
public class IPAddressSearchView extends View
{
   private final I18n i18n = LocalizationHelper.getI18n(IPAddressSearchView.class);

	private SearchResult searchResultWidget;
   private Button startButton;
   private LabeledText queryEditor; 
   private ZoneSelector zoneSelector;
   private boolean zoningEnabled;
   private Action actionStartSearch;

   /**
    * Create find by IP address view
    */
   public IPAddressSearchView()
   {
      super(LocalizationHelper.getI18n(IPAddressSearchView.class).tr("IP Address Search"), ResourceManager.getImageDescriptor("icons/tool-views/search_history.png"), "tools.ip-search", false);
   }

   /**
    * Post clone action
    */
   protected void postClone(View origin)
   {    
      super.postClone(origin);
      IPAddressSearchView view = (IPAddressSearchView)origin;
      zoneSelector.setZoneUIN(view.zoneSelector.getZoneUIN());
      queryEditor.setText(view.queryEditor.getText());
      searchResultWidget.copyResults(view.searchResultWidget);
   }   

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      final Composite searchBar = new Composite(parent, SWT.NONE);
      GridLayout gridLayout = new GridLayout();
      gridLayout.numColumns = 2;
      searchBar.setLayout(gridLayout);
      searchBar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      zoningEnabled = Registry.getSession().isZoningEnabled();
      if (zoningEnabled)
      {
         zoneSelector = new ZoneSelector(searchBar, SWT.NONE, false);
         zoneSelector.setLabel(i18n.tr("Zone"));
         zoneSelector.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

         PreferenceStore settings = PreferenceStore.getInstance();
         try
         {
            int uin = settings.getAsInteger("IPAddressSelection.ZoneUIN", 0);
            if (Registry.getSession().findZone(uin) != null)
               zoneSelector.setZoneUIN(uin);
         }
         catch(Exception e)
         {
            zoneSelector.setZoneUIN(0);
         }
      }

      queryEditor = new LabeledText(searchBar, SWT.NONE);
      queryEditor.setLabel(i18n.tr("Search string"));
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
               doSearch();
            }
         }
      });
      queryEditor.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      startButton = new Button(searchBar, SWT.PUSH);
      startButton.setImage(SharedIcons.IMG_EXECUTE);
      startButton.setText(i18n.tr("Start"));
      startButton.setToolTipText(i18n.tr("Start search"));
      startButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            doSearch();
         }
      });
      startButton.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, false, false));

      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

      searchResultWidget = new SearchResult(this, parent, SWT.NONE, getBaseId());
      searchResultWidget.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true, 2, 1));

      actionStartSearch = new Action(i18n.tr("&Start search"), SharedIcons.EXECUTE) {
         @Override
         public void run()
         {
            doSearch();
         }
      };
      addKeyBinding("F9", actionStartSearch);
	}

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionStartSearch);
      searchResultWidget.fillLocalToolBar(manager);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionStartSearch);
      manager.add(new Separator());
      searchResultWidget.fillLocalPullDown(manager);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
      queryEditor.setFocus();
	}

   /**
    * Do address search
    */
   private void doSearch()
	{
	   final InetAddress ipAddress;
      try
      {
         ipAddress = InetAddress.getByName(queryEditor.getText());
      }
      catch(UnknownHostException e)
      {
         MessageDialogHelper.openWarning(searchResultWidget.getShell(), i18n.tr("Warning"), i18n.tr("Please enter valid IP address!"));
         return;
      }

      final int zoneUIN = zoningEnabled ? zoneSelector.getZoneUIN() : 0;
      if (zoningEnabled)
      {
         PreferenceStore settings = PreferenceStore.getInstance();
         settings.set("IPAddressSelection.ZoneUIN", zoneUIN);
      }	   

      startButton.setEnabled(false);
      final NXCSession session = Registry.getSession();
      new Job(String.format(i18n.tr("Searching for IP address %s in the network"), ipAddress.toString()), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final ConnectionPoint cp = session.findConnectionPoint(zoneUIN, ipAddress);
            runInUIThread(() -> {
               searchResultWidget.showConnection(cp);
               startButton.setEnabled(true);
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format(i18n.tr("Search for IP address %s failed"), ipAddress.getHostAddress());
         }
      }.start();
	}
}
