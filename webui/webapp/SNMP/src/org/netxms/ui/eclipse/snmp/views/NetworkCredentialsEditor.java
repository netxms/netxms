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
package org.netxms.ui.eclipse.snmp.views;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SSHCredentials;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.SshKeyPair;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ZoneSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.EditSSHCredentialsDialog;
import org.netxms.ui.eclipse.snmp.dialogs.EditSnmpUsmCredentialsDialog;
import org.netxms.ui.eclipse.snmp.views.helpers.NetworkCredentials;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpUsmCredentialsLabelProvider;
import org.netxms.ui.eclipse.snmp.views.helpers.SshCredentialsLabelProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.MessageBar;
import org.netxms.ui.eclipse.widgets.Section;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Configurator for network discovery
 */
public class NetworkCredentialsEditor extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.NetworkCredentials"; //$NON-NLS-1$

	public static final int COLUMN_SNMP_USERNAME = 0;
   public static final int COLUMN_SNMP_AUTHENTICATION = 1;
   public static final int COLUMN_SNMP_ENCRYPTION = 2;
   public static final int COLUMN_SNMP_AUTH_PASSWORD = 3;
   public static final int COLUMN_SNMP_ENCRYPTION_PASSWORD = 4;
   public static final int COLUMN_SNMP_COMMENTS = 5;

   public static final int COLUMN_SSH_LOGIN = 0;
   public static final int COLUMN_SSH_PASSWORD = 1;
   public static final int COLUMN_SSH_KEY = 2;

   private NXCSession session;
	private boolean modified = false;
	private boolean bothModified = false;
   private boolean saveInProgress = false;
   private CompositeWithMessageBar contentWrapper;
   private Composite content;
	private TableViewer snmpCommunityList;
	private SortableTableViewer snmpUsmCredentialsList;
   private TableViewer snmpPortList;
   private TableViewer sharedSecretList;
   private TableViewer agentPortList;
   private TableViewer sshCredentialsList;
   private TableViewer sshPortList;
	private Action actionSave;
   private RefreshAction actionRefresh;
	private NetworkCredentials config;
	private ZoneSelector zoneSelector;
   private Display display;
   private int zoneUIN = NetworkCredentials.NETWORK_CONFIG_GLOBAL;
   private List<SshKeyPair> sshKeys;
   private SshCredentialsLabelProvider sshLabelProvider;

   /**
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
      config = new NetworkCredentials(session);
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createPartControl(Composite parent)
	{
      display = parent.getDisplay();

      contentWrapper = new CompositeWithMessageBar(parent, SWT.NONE);

      ScrolledComposite scroller = new ScrolledComposite(contentWrapper.getContent(), SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            content.layout(true, true);
            scroller.setMinSize(content.computeSize(scroller.getSize().x, SWT.DEFAULT));
         }
      });

      content = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.makeColumnsEqualWidth = true;
      content.setLayout(layout);
      content.setBackground(content.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));

      scroller.setContent(content);

		if (session.isZoningEnabled())
		{
         Composite headArea = new Composite(content, SWT.NONE);
         headArea.setBackground(content.getBackground());
   		headArea.setLayout(new GridLayout());
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = layout.numColumns;
         headArea.setLayoutData(gd);

         zoneSelector = new ZoneSelector(headArea, SWT.NONE, true);
         zoneSelector.setEmptySelectionText("Global");
         zoneSelector.setLabel("Select zone");
         zoneSelector.setBackground(headArea.getBackground());

         zoneSelector.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               zoneUIN = zoneSelector.getZoneUIN();
               updateFieldContent();
            }
         });

         gd = new GridData();
         gd.widthHint = 300;
         zoneSelector.setLayoutData(gd);

         Label separator = new Label(headArea, SWT.SEPARATOR | SWT.HORIZONTAL);
         separator.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
		}

		session.addListener(new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            int type = 0;
            switch(n.getCode())
            {
               case SessionNotification.COMMUNITIES_CONFIG_CHANGED:
                  type = NetworkCredentials.SNMP_COMMUNITIES;
                  break;
               case SessionNotification.USM_CONFIG_CHANGED:
                  type = NetworkCredentials.SNMP_USM_CREDENTIALS;
                  break;
               case SessionNotification.PORTS_CONFIG_CHANGED:
                  type = NetworkCredentials.SNMP_PORTS;
                  break;
               case SessionNotification.SECRET_CONFIG_CHANGED:
                  type = NetworkCredentials.AGENT_SECRETS;
                  break;
                  
            }
            if (type != 0)
            {
               final int configType = type;
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!config.isChanged(configType, (int)n.getSubCode()))
                     {
                        loadConfiguration(configType, (int)n.getSubCode());
                     }
                     else if (!saveInProgress)
                     {
                        contentWrapper.showMessage(MessageBar.WARNING,
                              "Network credentials are modified by other users. \"Refresh\" will discard local changes. \"Save\" will overwrite other users changes with local changes.");
                        bothModified = true;
                     }
                  }
               });
            }
         }
      });

		createSnmpCommunitySection();
      snmpPortList = createPortList(NetworkCredentials.SNMP_PORTS, "SNMP");
      createSnmpUsmCredSection();
      createSharedSecretList();
      agentPortList = createPortList(NetworkCredentials.AGENT_PORTS, "Agent");
      createSshCredentialsList();
      sshPortList = createPortList(NetworkCredentials.SSH_PORTS, "SSH");

		createActions();
		contributeToActionBars();

      // Load config
      loadConfiguration(NetworkCredentials.EVERYTHING, NetworkCredentials.ALL_ZONES);
	}

	/**
    * Load configuration from server
    * 
    * @param configId ID of SNMP configuration
    * @param zoneUIN of configuration
    */
	private void loadConfiguration(final int configId, final int zoneUIN)
	{
	   new ConsoleJob(Messages.get().NetworkCredentials_LoadingConfig, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyPair> sshKeys = session.getSshKeys(false);
            config.load(configId, zoneUIN);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  NetworkCredentialsEditor.this.sshKeys = sshKeys;
                  sshLabelProvider.setKeyList(sshKeys);
                  setConfig(config);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get(getDisplay()).NetworkCredentials_ErrorLoadingConfig;
         }
      }.start();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionSave = new Action(Messages.get().SnmpConfigurator_Save, SharedIcons.SAVE) {
			@Override
			public void run()
			{
				save();
			}
		};

      actionRefresh = new RefreshAction() {
         @Override
         public void run()
         {       
            hardRefresh();
         }
      };
	}

	/**
	 * Refresh view from button 
	 */
	private void hardRefresh()
	{
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Refresh network configuration",
               "This will discard all unsaved changes. Do you really want to continue?"))
            return;          
      }
      loadConfiguration(NetworkCredentials.EVERYTHING, NetworkCredentials.ALL_ZONES);  

      modified = false;
      bothModified = false;
      firePropertyChange(PROP_DIRTY);
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
      manager.add(actionRefresh);
	}

	/**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   private void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionSave);
      manager.add(actionRefresh);
   }

   /**
    * Create "SNMP Communities" section
    */
	private void createSnmpCommunitySection()
	{
      Section section = new Section(content, Messages.get().SnmpConfigurator_SectionCommunities, false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);

      snmpCommunityList = new TableViewer(clientArea, SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 150;
		snmpCommunityList.getTable().setLayoutData(gd);
		snmpCommunityList.setContentProvider(new ArrayContentProvider());
      final SensitiveDataLabelProvder labelProvider = new SensitiveDataLabelProvder();
      snmpCommunityList.setLabelProvider(labelProvider);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(Messages.get().SnmpConfigurator_Add);
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addCommunity();
			}
		});

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				removeCommunity();
			}
		});

      final ImageHyperlink linkCopy = new ImageHyperlink(controlArea, SWT.NONE);
      linkCopy.setText("Copy");
      linkCopy.setImage(SharedIcons.IMG_COPY);
      linkCopy.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkCopy.setLayoutData(gd);
      linkCopy.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            copyCommunity();
         }
      });

      final ImageHyperlink linkUp = new ImageHyperlink(controlArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      linkUp.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveCommunity(true);
         }
      });

      final ImageHyperlink linkDown = new ImageHyperlink(controlArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      linkDown.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveCommunity(false);
         }
      });

      final ImageHyperlink linkShowHide = new ImageHyperlink(controlArea, SWT.NONE);
      linkShowHide.setText("Reveal");
      linkShowHide.setImage(SharedIcons.IMG_SHOW);
      linkShowHide.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkShowHide.setLayoutData(gd);
      linkShowHide.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            labelProvider.setMaskMode(!labelProvider.isMaskMode());
            linkShowHide.setImage(labelProvider.isMaskMode() ? SharedIcons.IMG_SHOW : SharedIcons.IMG_HIDE);
            linkShowHide.setText(labelProvider.isMaskMode() ? "Reveal" : "Hide");
            snmpCommunityList.refresh();
         }
      });

      linkCopy.setEnabled(false);
      linkRemove.setEnabled(false);
      linkUp.setEnabled(false);
      linkDown.setEnabled(false);

      final Action actionCopy = new Action("&Copy", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyCommunity();
         }
      };
      final Action actionRemove = new Action("&Remove", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removeCommunity();
         }
      };
      final Action actionUp = new Action("&Up", SharedIcons.UP) {
         @Override
         public void run()
         {
            moveCommunity(true);
         }
      };
      final Action actionDown = new Action("&Down", SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveCommunity(false);
         }
      };
      MenuManager manager = new MenuManager();
      manager.add(actionCopy);
      manager.add(actionRemove);
      manager.add(actionUp);
      manager.add(actionDown);
      snmpCommunityList.getTable().setMenu(manager.createContextMenu(snmpCommunityList.getTable()));

      snmpCommunityList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = snmpCommunityList.getStructuredSelection();

            actionCopy.setEnabled(selection.size() == 1);
            actionRemove.setEnabled(!selection.isEmpty());
            actionUp.setEnabled(!selection.isEmpty());
            actionDown.setEnabled(!selection.isEmpty());

            linkCopy.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
            linkUp.setEnabled(!selection.isEmpty());
            linkDown.setEnabled(!selection.isEmpty());
         }
      });
	}

   /**
	 * Create "Address Filters" section
	 */
	private void createSnmpUsmCredSection()
	{
      Section section = new Section(content, Messages.get().SnmpConfigurator_SectionUSM, false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 3, 1));

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);
		
      final String[] names = { "User name", "Auth type", "Priv type", "Auth password", "Priv password", "Comments" };
		final int[] widths = { 100, 100, 100, 100, 100, 100 };
      snmpUsmCredentialsList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 150;
		snmpUsmCredentialsList.getTable().setLayoutData(gd);
		snmpUsmCredentialsList.setContentProvider(new ArrayContentProvider());
      final SnmpUsmCredentialsLabelProvider labelProvider = new SnmpUsmCredentialsLabelProvider();
      snmpUsmCredentialsList.setLabelProvider(labelProvider);
		snmpUsmCredentialsList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredentials();
         }
      });

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
		linkAdd.setText(Messages.get().SnmpConfigurator_Add);
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkAdd.setLayoutData(gd);
		linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addUsmCredentials();
			}
		});

      final ImageHyperlink linkEdit = new ImageHyperlink(controlArea, SWT.NONE);
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkEdit.setLayoutData(gd);
      linkEdit.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editUsmCredentials();
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		linkRemove.setLayoutData(gd);
		linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				removeUsmCredentials();
			}
		});

      final ImageHyperlink linkUp = new ImageHyperlink(controlArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      linkUp.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
           moveUsmCredentials(true);
         }
      });

      final ImageHyperlink linkDown = new ImageHyperlink(controlArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      linkDown.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveUsmCredentials(false);
         }
      });

      final ImageHyperlink linkShowHide = new ImageHyperlink(controlArea, SWT.NONE);
      linkShowHide.setText("Reveal");
      linkShowHide.setImage(SharedIcons.IMG_SHOW);
      linkShowHide.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkShowHide.setLayoutData(gd);
      linkShowHide.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            labelProvider.setMaskMode(!labelProvider.isMaskMode());
            linkShowHide.setImage(labelProvider.isMaskMode() ? SharedIcons.IMG_SHOW : SharedIcons.IMG_HIDE);
            linkShowHide.setText(labelProvider.isMaskMode() ? "Reveal" : "Hide");
            snmpUsmCredentialsList.refresh();
         }
      });

      linkEdit.setEnabled(false);
      linkRemove.setEnabled(false);
      linkUp.setEnabled(false);
      linkDown.setEnabled(false);

      final Action actionEdit = new Action("&Edit...", SharedIcons.EDIT) {
         @Override
         public void run()
         {
            editUsmCredentials();
         }
      };
      final Action actionRemove = new Action("&Remove", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removeUsmCredentials();
         }
      };
      final Action actionUp = new Action("&Up", SharedIcons.UP) {
         @Override
         public void run()
         {
            moveUsmCredentials(true);
         }
      };
      final Action actionDown = new Action("&Down", SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveUsmCredentials(false);
         }
      };
      MenuManager manager = new MenuManager();
      manager.add(actionEdit);
      manager.add(actionRemove);
      manager.add(actionUp);
      manager.add(actionDown);
      snmpUsmCredentialsList.getTable().setMenu(manager.createContextMenu(snmpUsmCredentialsList.getTable()));

      snmpUsmCredentialsList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = snmpUsmCredentialsList.getStructuredSelection();

            actionEdit.setEnabled(selection.size() == 1);
            actionRemove.setEnabled(!selection.isEmpty());
            actionUp.setEnabled(!selection.isEmpty());
            actionDown.setEnabled(!selection.isEmpty());

            linkEdit.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
            linkUp.setEnabled(!selection.isEmpty());
            linkDown.setEnabled(!selection.isEmpty());
         }
      });
	}

   /**
    * Create "Agent shared secrets" section
    */
   private void createSharedSecretList()
   {
      Section section = new Section(content, "Agent shared secrets", false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);

      sharedSecretList = new TableViewer(clientArea, SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 150;
      sharedSecretList.getTable().setLayoutData(gd);
      sharedSecretList.setContentProvider(new ArrayContentProvider());
      final SensitiveDataLabelProvder labelProvider = new SensitiveDataLabelProvder();
      sharedSecretList.setLabelProvider(labelProvider);

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addSharedSecret();
         }
      });
      
      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeSharedSecret();
         }
      });

      final ImageHyperlink linkCopy = new ImageHyperlink(controlArea, SWT.NONE);
      linkCopy.setText("Copy");
      linkCopy.setImage(SharedIcons.IMG_COPY);
      linkCopy.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkCopy.setLayoutData(gd);
      linkCopy.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            copySharedSecret();
         }
      });

      final ImageHyperlink linkUp = new ImageHyperlink(controlArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      linkUp.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveSharedSecret(true);
         }
      });

      final ImageHyperlink linkDown = new ImageHyperlink(controlArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      linkDown.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveSharedSecret(false);
         }
      });

      final ImageHyperlink linkShowHide = new ImageHyperlink(controlArea, SWT.NONE);
      linkShowHide.setText("Reveal");
      linkShowHide.setImage(SharedIcons.IMG_SHOW);
      linkShowHide.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkShowHide.setLayoutData(gd);
      linkShowHide.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            labelProvider.setMaskMode(!labelProvider.isMaskMode());
            linkShowHide.setImage(labelProvider.isMaskMode() ? SharedIcons.IMG_SHOW : SharedIcons.IMG_HIDE);
            linkShowHide.setText(labelProvider.isMaskMode() ? "Reveal" : "Hide");
            sharedSecretList.refresh();
         }
      });

      linkCopy.setEnabled(false);
      linkRemove.setEnabled(false);
      linkUp.setEnabled(false);
      linkDown.setEnabled(false);

      final Action actionCopy = new Action("&Copy", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySharedSecret();
         }
      };
      final Action actionRemove = new Action("&Remove", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removeSharedSecret();
         }
      };
      final Action actionUp = new Action("&Up", SharedIcons.UP) {
         @Override
         public void run()
         {
            moveSharedSecret(true);
         }
      };
      final Action actionDown = new Action("&Down", SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveSharedSecret(false);
         }
      };
      MenuManager manager = new MenuManager();
      manager.add(actionCopy);
      manager.add(actionRemove);
      manager.add(actionUp);
      manager.add(actionDown);
      sharedSecretList.getTable().setMenu(manager.createContextMenu(sharedSecretList.getTable()));

      sharedSecretList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = sharedSecretList.getStructuredSelection();

            actionCopy.setEnabled(selection.size() == 1);
            actionRemove.setEnabled(!selection.isEmpty());
            actionUp.setEnabled(!selection.isEmpty());
            actionDown.setEnabled(!selection.isEmpty());

            linkCopy.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
            linkUp.setEnabled(!selection.isEmpty());
            linkDown.setEnabled(!selection.isEmpty());
         }
      });
   }

   /**
    * Create "SSH Credentials" section
    */
   private void createSshCredentialsList()
   {
      Section section = new Section(content, "SSH credentials", false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);

      final String[] names = { "Login", "Password", "Key" };
      final int[] widths = { 150, 150, 150 };
      sshCredentialsList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 150;
      sshCredentialsList.getTable().setLayoutData(gd);
      sshCredentialsList.setContentProvider(new ArrayContentProvider());
      sshLabelProvider = new SshCredentialsLabelProvider();
      sshCredentialsList.setLabelProvider(sshLabelProvider);
      sshCredentialsList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editSshCredentials();
         }
      });

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addSshCredentials();
         }
      });

      final ImageHyperlink linkEdit = new ImageHyperlink(controlArea, SWT.NONE);
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
      linkEdit.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkEdit.setLayoutData(gd);
      linkEdit.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editSshCredentials();
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removeSshCredentials();
         }
      });

      final ImageHyperlink linkCopy = new ImageHyperlink(controlArea, SWT.NONE);
      linkCopy.setText("Copy");
      linkCopy.setImage(SharedIcons.IMG_COPY);
      linkCopy.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkCopy.setLayoutData(gd);
      linkCopy.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            copySshPassword();
         }
      });

      final ImageHyperlink linkUp = new ImageHyperlink(controlArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      linkUp.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveSshCredentials(true);
         }
      });

      final ImageHyperlink linkDown = new ImageHyperlink(controlArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      linkDown.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            moveSshCredentials(false);
         }
      });

      final ImageHyperlink linkShowHide = new ImageHyperlink(controlArea, SWT.NONE);
      linkShowHide.setText("Reveal");
      linkShowHide.setImage(SharedIcons.IMG_SHOW);
      linkShowHide.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkShowHide.setLayoutData(gd);
      linkShowHide.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            sshLabelProvider.setMaskMode(!sshLabelProvider.isMaskMode());
            linkShowHide.setImage(sshLabelProvider.isMaskMode() ? SharedIcons.IMG_SHOW : SharedIcons.IMG_HIDE);
            linkShowHide.setText(sshLabelProvider.isMaskMode() ? "Reveal" : "Hide");
            sshCredentialsList.refresh();
         }
      });

      linkEdit.setEnabled(false);
      linkCopy.setEnabled(false);
      linkRemove.setEnabled(false);
      linkUp.setEnabled(false);
      linkDown.setEnabled(false);

      final Action actionCopy = new Action("&Copy password", SharedIcons.COPY) {
         @Override
         public void run()
         {
            copySshPassword();
         }
      };
      final Action actionEdit = new Action("&Edit...", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            editSshCredentials();
         }
      };
      final Action actionRemove = new Action("&Remove", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removeSshCredentials();
         }
      };
      final Action actionUp = new Action("&Up", SharedIcons.UP) {
         @Override
         public void run()
         {
            moveSshCredentials(true);
         }
      };
      final Action actionDown = new Action("&Down", SharedIcons.DOWN) {
         @Override
         public void run()
         {
            moveSshCredentials(false);
         }
      };
      MenuManager manager = new MenuManager();
      manager.add(actionEdit);
      manager.add(actionCopy);
      manager.add(actionRemove);
      manager.add(actionUp);
      manager.add(actionDown);
      sshCredentialsList.getTable().setMenu(manager.createContextMenu(sshCredentialsList.getTable()));

      sshCredentialsList.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = sshCredentialsList.getStructuredSelection();

            actionEdit.setEnabled(selection.size() == 1);
            actionCopy.setEnabled(selection.size() == 1);
            actionRemove.setEnabled(!selection.isEmpty());
            actionUp.setEnabled(!selection.isEmpty());
            actionDown.setEnabled(!selection.isEmpty());

            linkEdit.setEnabled(selection.size() == 1);
            linkCopy.setEnabled(selection.size() == 1);
            linkRemove.setEnabled(!selection.isEmpty());
            linkUp.setEnabled(!selection.isEmpty());
            linkDown.setEnabled(!selection.isEmpty());
         }
      });
   }

   /**
    * Create "XXX ports" section
    */
   private TableViewer createPortList(int portType, String typeName)
   {
      Section section = new Section(content, String.format("%s ports", typeName), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));

      Composite clientArea = section.getClient();
      clientArea.setBackground(content.getBackground());
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 0;
      clientArea.setLayout(layout);

      final TableViewer viewer = new TableViewer(clientArea, SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 150;
      viewer.getTable().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setData("PortType", Integer.valueOf(portType));

      Label separator = new Label(clientArea, SWT.SEPARATOR | SWT.VERTICAL);
      gd = new GridData();
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      separator.setLayoutData(gd);

      Composite controlArea = new Composite(clientArea, SWT.NONE);
      controlArea.setBackground(clientArea.getBackground());
      controlArea.setLayout(new GridLayout());
      controlArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, true));

      final ImageHyperlink linkAdd = new ImageHyperlink(controlArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      linkAdd.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addPort(viewer, typeName);
         }
      });

      final ImageHyperlink linkRemove = new ImageHyperlink(controlArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      linkRemove.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removePort(viewer);
         }
      });

      final ImageHyperlink linkUp = new ImageHyperlink(controlArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      linkUp.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            movePort(viewer, true);
         }
      });

      final ImageHyperlink linkDown = new ImageHyperlink(controlArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      linkDown.setBackground(clientArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            movePort(viewer, false);
         }
      });

      linkRemove.setEnabled(false);
      linkUp.setEnabled(false);
      linkDown.setEnabled(false);

      final Action actionRemove = new Action("&Remove", SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            removePort(viewer);
         }
      };
      final Action actionUp = new Action("&Up", SharedIcons.UP) {
         @Override
         public void run()
         {
            movePort(viewer, true);
         }
      };
      final Action actionDown = new Action("&Down", SharedIcons.DOWN) {
         @Override
         public void run()
         {
            movePort(viewer, false);
         }
      };
      MenuManager manager = new MenuManager();
      manager.add(actionRemove);
      manager.add(actionUp);
      manager.add(actionDown);
      viewer.getTable().setMenu(manager.createContextMenu(viewer.getTable()));

      viewer.addSelectionChangedListener(new ISelectionChangedListener() {
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = viewer.getStructuredSelection();

            actionRemove.setEnabled(!selection.isEmpty());
            actionUp.setEnabled(!selection.isEmpty());
            actionDown.setEnabled(!selection.isEmpty());

            linkRemove.setEnabled(!selection.isEmpty());
            linkUp.setEnabled(!selection.isEmpty());
            linkDown.setEnabled(!selection.isEmpty());
         }
      });

      return viewer;
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
    */
	@Override
	public void setFocus()
	{
      content.setFocus();
	}

	/**
	 * @param config
	 */
	private void setConfig(NetworkCredentials config)
	{
		this.config = config;
		updateFieldContent();
	}

	/**
	 * Update filed content
	 */
	private void updateFieldContent()
	{
      snmpCommunityList.setInput(config.getCommunities(zoneUIN));
      snmpUsmCredentialsList.setInput(config.getUsmCredentials(zoneUIN));
      snmpPortList.setInput(config.getPorts(NetworkCredentials.SNMP_PORTS, zoneUIN));
      sharedSecretList.setInput(config.getSharedSecrets(zoneUIN));
      agentPortList.setInput(config.getPorts(NetworkCredentials.AGENT_PORTS, zoneUIN));
      sshCredentialsList.setInput(config.getSshCredentials(zoneUIN));
      sshPortList.setInput(config.getPorts(NetworkCredentials.SSH_PORTS, zoneUIN));
	}

	/**
	 * Mark view as modified
	 */
	private void setModified(int id)
	{
	   config.setConfigUpdate(zoneUIN, id);
		if (!modified)
		{
			modified = true;
			firePropertyChange(PROP_DIRTY);
		}
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
    */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		try
		{
         config.save();
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getSite().getShell(), Messages.get().SnmpConfigurator_Error, 
			      String.format(Messages.get().NetworkCredentials_CannotSaveConfig, e.getLocalizedMessage()));
		}
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#doSaveAs()
    */
	@Override
	public void doSaveAs()
	{
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isDirty()
    */
	@Override
	public boolean isDirty()
	{
		return modified;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveAsAllowed()
    */
	@Override
	public boolean isSaveAsAllowed()
	{
		return false;
	}

   /**
    * @see org.eclipse.ui.ISaveablePart#isSaveOnCloseNeeded()
    */
	@Override
	public boolean isSaveOnCloseNeeded()
	{
		return modified;
	}
	
	/**
	 * Save settings
	 */
	private void save()
	{
	   if (bothModified && modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Save network credential",
               "Network credentials already are modified by other users. Do you really want to continue and overwrite other users changes?\n"))
            return;
         contentWrapper.hideMessage();   
      }	  

      saveInProgress = true;
		new ConsoleJob(Messages.get().NetworkCredentials_SaveConfig, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
            config.save();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						modified = false;
			         bothModified = false;
						firePropertyChange(PROP_DIRTY);
                  saveInProgress = false;
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return Messages.get(getDisplay()).NetworkCredentials_ErrorSavingConfig;
			}

         @Override
         protected void jobFinalize()
         {
            saveInProgress = false;
         }
		}.start();
	}

	/**
	 * Add SNMP community to the list
	 */
	private void addCommunity()
	{
		InputDialog dlg = new InputDialog(getSite().getShell(), Messages.get().SnmpConfigurator_AddCommunity, 
            Messages.get().SnmpConfigurator_AddCommunityDescr, "", null); //$NON-NLS-1$
		if (dlg.open() == Window.OK)
		{
			String s = dlg.getValue();
			config.addCommunityString(s, zoneUIN);
         snmpCommunityList.setInput(config.getCommunities(zoneUIN));
         setModified(NetworkCredentials.SNMP_COMMUNITIES);
		}
	}

	/**
	 * Remove selected SNMP communities
	 */
	private void removeCommunity()
	{
		final List<String> list = config.getCommunities(zoneUIN);
      IStructuredSelection selection = snmpCommunityList.getStructuredSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
			{
				list.remove(o);
			}
			snmpCommunityList.setInput(list);
			setModified(NetworkCredentials.SNMP_COMMUNITIES);
		}
	}
   
   /**
    * Copy selected SNMP community to clipboard
    */
   private void copyCommunity()
   {
      IStructuredSelection selection = snmpCommunityList.getStructuredSelection();
      if (selection.size() == 1)
      {
         WidgetHelper.copyToClipboard((String)selection.getFirstElement());
      }
   }

	/**
	 * Move community string in priority
	 * 
	 * @param up true to move up or false to move down
	 */
   protected void moveCommunity(boolean up)
   {
      final List<String> list = config.getCommunities(zoneUIN);
      IStructuredSelection selection = snmpCommunityList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = list.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;
               
               Collections.swap(list, index - 1, index);
            }
            else
            {
               if ((index + 1) == list.size())
                  return;
               
               Collections.swap(list, index + 1, index);
            }
         }
         snmpCommunityList.setInput(list);
         setModified(NetworkCredentials.SNMP_COMMUNITIES);
      }
   }

	/**
	 * Add SNMP USM credentials to the list
	 */
	private void addUsmCredentials()
	{
		EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getSite().getShell(), null);
		if (dlg.open() == Window.OK)
		{
			SnmpUsmCredential cred = dlg.getCredentials();
			cred.setZoneId((int)zoneUIN);
         config.addUsmCredentials(cred, zoneUIN);
         snmpUsmCredentialsList.setInput(config.getUsmCredentials(zoneUIN));
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
		}
	}
   
   /**
    * Edit SNMP USM credential
    */
   private void editUsmCredentials()
   {
      IStructuredSelection selection = snmpUsmCredentialsList.getStructuredSelection();
      if (selection.size() != 1)
         return;
      SnmpUsmCredential cred = (SnmpUsmCredential)selection.getFirstElement();
      EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getSite().getShell(), cred);
      if (dlg.open() == Window.OK)
      {
         final List<SnmpUsmCredential> list = config.getUsmCredentials(zoneUIN);
         snmpUsmCredentialsList.setInput(list.toArray());
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }
	
	/**
	 * Remove selected SNMP USM credentials
	 */
	private void removeUsmCredentials()
	{
		final List<SnmpUsmCredential> list = config.getUsmCredentials(zoneUIN);
      IStructuredSelection selection = snmpUsmCredentialsList.getStructuredSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
			{
				list.remove(o);
			}
			snmpUsmCredentialsList.setInput(list.toArray());
			setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
		}
	}

	/**
	 * Move SNMP USM credential
	 * 
	 * @param up true if up, false if down
	 */
   protected void moveUsmCredentials(boolean up)
   {
      final List<SnmpUsmCredential> list = config.getUsmCredentials(zoneUIN);
      IStructuredSelection selection = snmpUsmCredentialsList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = list.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;
               
               Collections.swap(list, index - 1, index);
            }
            else
            {
               if ((index + 1) == list.size())
                  return;
               
               Collections.swap(list, index + 1, index);
            }
         }
         snmpUsmCredentialsList.setInput(list);
         setModified(NetworkCredentials.SNMP_USM_CREDENTIALS);
      }
   }
	
   /**
    * Add agent shared secret
    */
   protected void addSharedSecret()
   {
      InputDialog dlg = new InputDialog(getSite().getShell(), "Add shared secret", "Please enter shared secret", "", null);
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         config.addSharedSecret(value, zoneUIN);
         sharedSecretList.setInput(config.getSharedSecrets(zoneUIN));
         setModified(NetworkCredentials.AGENT_SECRETS);
      }
   }

   /**
    * Remove agent shared secret
    */
   protected void removeSharedSecret()
   {
      final List<String> list = config.getSharedSecrets(zoneUIN);
      IStructuredSelection selection = sharedSecretList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         sharedSecretList.setInput(list.toArray());
         setModified(NetworkCredentials.AGENT_SECRETS);
      }
   }

   /**
    * Copy agent shared secret to clipboard
    */
   protected void copySharedSecret()
   {
      IStructuredSelection selection = sharedSecretList.getStructuredSelection();
      if (selection.size() == 1)
      {
         WidgetHelper.copyToClipboard((String)selection.getFirstElement());
      }
   }

   /**
    * Move up or down agent shared secret
    * 
    * @param up true if up, false if down
    */
   protected void moveSharedSecret(boolean up)
   {
      final List<String> list = config.getSharedSecrets(zoneUIN);
      IStructuredSelection selection = sharedSecretList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = list.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;
               
               Collections.swap(list, index - 1, index);
            }
            else
            {
               if ((index + 1) == list.size())
                  return;
               
               Collections.swap(list, index + 1, index);
            }
         }
         sharedSecretList.setInput(list);
         setModified(NetworkCredentials.AGENT_SECRETS);
      }
   }

   /**
    * Add SSH credentials to the list
    */
   private void addSshCredentials()
   {
      EditSSHCredentialsDialog dlg = new EditSSHCredentialsDialog(getSite().getShell(), null, sshKeys);
      if (dlg.open() == Window.OK)
      {
         SSHCredentials cred = dlg.getCredentials();
         config.addSshCredentials(cred, zoneUIN);
         sshCredentialsList.setInput(config.getSshCredentials(zoneUIN));
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Edit SSH credential
    */
   private void editSshCredentials()
   {
      IStructuredSelection selection = sshCredentialsList.getStructuredSelection();
      if (selection.size() != 1)
         return;
      SSHCredentials cred = (SSHCredentials)selection.getFirstElement();
      EditSSHCredentialsDialog dlg = new EditSSHCredentialsDialog(getSite().getShell(), cred, sshKeys);
      if (dlg.open() == Window.OK)
      {
         final List<SSHCredentials> list = config.getSshCredentials(zoneUIN);
         sshCredentialsList.setInput(list.toArray());
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Remove selected SSH credentials
    */
   private void removeSshCredentials()
   {
      final List<SSHCredentials> list = config.getSshCredentials(zoneUIN);
      IStructuredSelection selection = sshCredentialsList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         sshCredentialsList.setInput(list.toArray());
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Copy selected SSH password
    */
   private void copySshPassword()
   {
      IStructuredSelection selection = sshCredentialsList.getStructuredSelection();
      if (selection.size() == 1)
      {
         WidgetHelper.copyToClipboard(((SSHCredentials)selection.getFirstElement()).getPassword());
      }
   }

   /**
    * Move SSH credential
    * 
    * @param up true if up, false if down
    */
   protected void moveSshCredentials(boolean up)
   {
      final List<SSHCredentials> list = config.getSshCredentials(zoneUIN);
      IStructuredSelection selection = sshCredentialsList.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = list.indexOf(o);
            if (up)
            {
               if (index < 1)
                  return;

               Collections.swap(list, index - 1, index);
            }
            else
            {
               if ((index + 1) == list.size())
                  return;

               Collections.swap(list, index + 1, index);
            }
         }
         sshCredentialsList.setInput(list);
         setModified(NetworkCredentials.SSH_CREDENTIALS);
      }
   }

   /**
    * Add port to the list
    * 
    * @param type port type
    */
   private void addPort(TableViewer viewer, String typeName)
   {
      int portType = (Integer)viewer.getData("PortType");
      InputDialog dlg = new InputDialog(getSite().getShell(), String.format("Add %s port", typeName), 
            String.format("Please enter %s port", typeName), "", null); // $NON-NLS-2$
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         config.addPort(portType, Integer.parseInt(value), zoneUIN);
         viewer.setInput(config.getPorts(portType, zoneUIN));
         setModified(portType);
      }
   }

   /**
    * Remove selected port
    * 
    * @param type port type
    */
   private void removePort(TableViewer viewer)
   {
      int portType = (Integer)viewer.getData("PortType");
      final List<Integer> list = config.getPorts(portType, zoneUIN);
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove((Integer)o);
         }
         viewer.setInput(list.toArray());
         setModified(portType);
      }
   }

   /**
    * Move port
    * 
    * @param type port type
    * @param up true if up, false if down
    */
   protected void movePort(TableViewer viewer, boolean up)
   {
      int portType = (Integer)viewer.getData("PortType");
      final List<Integer> list = config.getPorts(portType, zoneUIN);
      IStructuredSelection selection = viewer.getStructuredSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            int index = list.indexOf((Integer)o);
            if (up)
            {
               if (index < 1)
                  return;

               Collections.swap(list, index - 1, index);
            }
            else
            {
               if ((index + 1) == list.size())
                  return;

               Collections.swap(list, index + 1, index);
            }
         }
         viewer.setInput(list);
         setModified(portType);
      }
   }

   /**
    * Label provider for sensitive data
    */
   private static class SensitiveDataLabelProvder extends LabelProvider
   {
      private boolean maskMode = true;

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getText(java.lang.Object)
       */
      @Override
      public String getText(Object element)
      {
         return maskMode ? WidgetHelper.maskPassword(((String)element)) : (String)element;
      }

      /**
       * @return the maskMode
       */
      public boolean isMaskMode()
      {
         return maskMode;
      }

      /**
       * @param maskMode the maskMode to set
       */
      public void setMaskMode(boolean maskMode)
      {
         this.maskMode = maskMode;
      }
   };
}
