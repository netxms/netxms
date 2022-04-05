/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.ISaveablePart;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.SshCredential;
import org.netxms.client.SshKeyPair;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ZoneSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.AddSshCredDialog;
import org.netxms.ui.eclipse.snmp.dialogs.AddUsmCredDialog;
import org.netxms.ui.eclipse.snmp.views.helpers.NetworkConfig;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpUsmLabelProvider;
import org.netxms.ui.eclipse.snmp.views.helpers.SshCredLabelProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.widgets.CompositeWithMessageBar;
import org.netxms.ui.eclipse.widgets.MessageBar;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Configurator for network discovery
 */
public class NetworkCredentials extends ViewPart implements ISaveablePart
{
	public static final String ID = "org.netxms.ui.eclipse.snmp.views.NetworkCredentials"; //$NON-NLS-1$

	public static final int USM_CRED_USER_NAME = 0;
   public static final int USM_CRED_AUTHENTICATION = 1;
   public static final int USM_CRED_ENCRYPTION = 2;
   public static final int USM_CRED_AUTH_PASSWORD = 3;
   public static final int USM_CRED_ENC_PASSWORD = 4;
   public static final int USM_CRED_COMMENTS = 5;
	
   public static final int SSH_CRED_LOGIN = 0;
   public static final int SSH_CRED_PASSWORD = 1;
   public static final int SSH_CRED_KEY = 2;

	private NXCSession session;
	private boolean modified = false;
	private boolean bothModified = false;
   private boolean saveInProgress = false;
   private CompositeWithMessageBar content;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private TableViewer snmpCommunityList;
	private SortableTableViewer snmpUsmCredList;
   private TableViewer snmpPortList;
   private TableViewer sharedSecretList;
   private TableViewer agentPortList;
   private TableViewer sshCredList;
   private TableViewer sshPortList;
	private Action actionSave;
   private RefreshAction actionRefresh;
	private NetworkConfig config;
	private ZoneSelector zoneSelector;
   private Display display;
	private long zoneUIN = NetworkConfig.NETWORK_CONFIG_GLOBAL;
   private List<SshKeyPair> keyList;
   private SshCredLabelProvider sshLabelProvider;


	/* (non-Javadoc)
    * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
    */
   @Override
   public void init(IViewSite site) throws PartInitException
   {
      super.init(site);
      session = ConsoleSharedData.getSession();
      config = new NetworkConfig(session);
   }

   private void setPortList(int type, TableViewer tw)
   {
      switch(type)
      {
         case NetworkConfig.SNMP_PORTS:
            snmpPortList = tw;
            break;
         case NetworkConfig.AGENT_PORTS:
            agentPortList = tw;
            break;
         case NetworkConfig.SSH_PORTS:
            sshPortList = tw;
            break;
      }
   }

   private TableViewer getPortList(int type)
   {
      switch(type)
      {
         case NetworkConfig.SNMP_PORTS:
            return snmpPortList;
         case NetworkConfig.AGENT_PORTS:
            return agentPortList;
         case NetworkConfig.SSH_PORTS:
            return sshPortList;
      }
      return null;
   }

   private String getPortTypeAsString(int type)
   {
      switch(type)
      {
         case NetworkConfig.SNMP_PORTS:
            return "SNMP";
         case NetworkConfig.AGENT_PORTS:
            return "agent";
         case NetworkConfig.SSH_PORTS:
            return "SSH";
      }
      return null;
   }

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
      display = parent.getDisplay();
	   
      content = new CompositeWithMessageBar(parent, SWT.NONE);
		toolkit = new FormToolkit(getSite().getShell().getDisplay());
		form = toolkit.createScrolledForm(content.getContent());
      form.setText(Messages.get().NetworkCredentials_FormTitle);
	
		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		if (session.isZoningEnabled())
		{
         toolkit.decorateFormHeading(form.getForm());
   		Composite headArea = toolkit.createComposite(form.getForm().getHead());
   		headArea.setLayout(new GridLayout());
         zoneSelector = new ZoneSelector(headArea, SWT.NONE, true);
         zoneSelector.setEmptySelectionText("Global");
         zoneSelector.setLabel("Select zone");
   
         GridData gd = new GridData();
         gd.widthHint = 300;
         zoneSelector.setLayoutData(gd);
         form.setHeadClient(headArea);
         zoneSelector.addModifyListener(new ModifyListener() {
            
            @Override
            public void modifyText(ModifyEvent e)
            {
               zoneUIN = zoneSelector.getZoneUIN();
               updateFieldContent();
            }
         });
		}
		
		session.addListener(new SessionListener() {
         
         @Override
         public void notificationHandler(SessionNotification n)
         {
            int type = 0;
            switch(n.getCode())
            {
               case SessionNotification.COMMUNITIES_CONFIG_CHANGED:
                  type = NetworkConfig.COMMUNITIES;
                  break;
               case SessionNotification.USM_CONFIG_CHANGED:
                  type = NetworkConfig.USM;
                  break;
               case SessionNotification.PORTS_CONFIG_CHANGED:
                  type = NetworkConfig.SNMP_PORTS;
                  break;
               case SessionNotification.SECRET_CONFIG_CHANGED:
                  type = NetworkConfig.AGENT_SECRETS;
                  break;
                  
            }     
            if(type != 0)
            {
               final int configType = type;
               display.asyncExec(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!config.isChanged(configType, (int)n.getSubCode()))
                     {
                        loadSnmpConfig(configType, (int)n.getSubCode());
                     }
                     else if (!saveInProgress)
                     {
                        content.showMessage(MessageBar.WARNING,
                              "Network credentials are modified by other users. \"Refresh\" will discard local changes. \"Save\" will overwrite other users changes with local changes.");
                        bothModified = true;
                     }
                  }
               });
            }
         }
      });
      
		createSnmpCommunitySection();
      createPortList(NetworkConfig.SNMP_PORTS, "SNMP ports", "description"); // TODO Filipp had an idea what to write in descriptions
      createSnmpUsmCredSection();
      createSharedSecretList();
      createPortList(NetworkConfig.AGENT_PORTS, "Agent ports", "description");
      createSshCredentialsList();
      createPortList(NetworkConfig.SSH_PORTS, "SSH ports", "description");
		
		createActions();
		contributeToActionBars();

      // Load config
      loadSnmpConfig(NetworkConfig.ALL_CONFIGS, NetworkConfig.ALL_ZONES);
	}
	
	/**
    * Load SNMP configuration
    * 
    * @param configId ID of SNMP configuration
    * @param zoneUIN of configuration
    */
	private void loadSnmpConfig(final int configId, final int zoneUIN)
	{
	   new ConsoleJob(Messages.get().NetworkCredentials_LoadingConfig, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            keyList = session.getSshKeys(false);
            sshLabelProvider.setKeyList(keyList);
            config.load(configId, zoneUIN);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  setConfig(config);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return Messages.get().NetworkCredentials_ErrorLoadingConfig;
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
      loadSnmpConfig(NetworkConfig.ALL_CONFIGS, NetworkConfig.ALL_ZONES);  
      
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
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText(Messages.get().SnmpConfigurator_SectionCommunities);
		section.setDescription(Messages.get().SnmpConfigurator_SectionCommunitiesDescr);
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		snmpCommunityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpCommunityList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 4;
		gd.heightHint = 150;
		snmpCommunityList.getTable().setLayoutData(gd);
		snmpCommunityList.setContentProvider(new ArrayContentProvider());
		
		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText(Messages.get().SnmpConfigurator_Add);
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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
		
		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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
      
      final ImageHyperlink linkUp = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
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
      
      final ImageHyperlink linkDown = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
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
	}

   /**
	 * Create "Address Filters" section
	 */
	private void createSnmpUsmCredSection()
	{
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText(Messages.get().SnmpConfigurator_SectionUSM);
		section.setDescription(Messages.get().SnmpConfigurator_SectionUSMDescr);
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		td.colspan = 2;
		section.setLayoutData(td);
		
		Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);
		section.setClient(clientArea);
		
		final String[] names = { "User name", "Auth type", "Priv type", "Auth secret", "Priv secret", "Comments" };
		final int[] widths = { 100, 100, 100, 100, 100, 100 };
		snmpUsmCredList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		toolkit.adapt(snmpUsmCredList.getTable());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 5;
		gd.heightHint = 150;
		snmpUsmCredList.getTable().setLayoutData(gd);
		snmpUsmCredList.setContentProvider(new ArrayContentProvider());
		snmpUsmCredList.setLabelProvider(new SnmpUsmLabelProvider());
		snmpUsmCredList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredentials();
         }
      });

		final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkAdd.setText(Messages.get().SnmpConfigurator_Add);
		linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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

      final ImageHyperlink linkEdit = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
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

		final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
		linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
		linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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

      final ImageHyperlink linkUp = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
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
      
      final ImageHyperlink linkDown = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
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
	}

   /**
    * Create "Agent shared secrets" section
    */
   private void createSharedSecretList()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText("Agent shared secrets");
      section.setDescription("Agent shared secrets used in the network");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);
      
      sharedSecretList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      toolkit.adapt(sharedSecretList.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 150;
      sharedSecretList.getTable().setLayoutData(gd);
      sharedSecretList.setContentProvider(new ArrayContentProvider());

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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
      
      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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
      
      final ImageHyperlink linkUp = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
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
      
      final ImageHyperlink linkDown = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
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
   }

   /**
    * Create "SSH Credentials" section
    */
   private void createSshCredentialsList()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText("SSH credentials");
      section.setDescription("SSH credentials used in the network");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      // td.colspan = 2;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      final String[] names = { "Login", "Password", "Key" };
      final int[] widths = { 150, 150, 150 };
      sshCredList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      toolkit.adapt(sshCredList.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 5;
      gd.heightHint = 150;
      sshCredList.getTable().setLayoutData(gd);
      sshCredList.setContentProvider(new ArrayContentProvider());
      sshLabelProvider = new SshCredLabelProvider();
      sshCredList.setLabelProvider(sshLabelProvider);
      sshCredList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredentials();
         }
      });

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
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

      final ImageHyperlink linkEdit = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkEdit.setText("Edit...");
      linkEdit.setImage(SharedIcons.IMG_EDIT);
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

      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
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

      final ImageHyperlink linkUp = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
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

      final ImageHyperlink linkDown = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
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
   }

   /**
    * Create "XXX ports" section
    */
   private void createPortList(int type, String title, String description)
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(title);
      section.setDescription(description);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);

      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);

      setPortList(type, new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION));
      toolkit.adapt(getPortList(type).getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 150;
      getPortList(type).getTable().setLayoutData(gd);
      getPortList(type).setContentProvider(new ArrayContentProvider());

      final ImageHyperlink linkAdd = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(Messages.get().SnmpConfigurator_Add);
      linkAdd.setImage(SharedIcons.IMG_ADD_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkAdd.setLayoutData(gd);
      linkAdd.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addPort(type);
         }
      });

      final ImageHyperlink linkRemove = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(Messages.get().SnmpConfigurator_Remove);
      linkRemove.setImage(SharedIcons.IMG_DELETE_OBJECT);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkRemove.setLayoutData(gd);
      linkRemove.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            removePort(type);
         }
      });

      final ImageHyperlink linkUp = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkUp.setText("Up");
      linkUp.setImage(SharedIcons.IMG_UP);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkUp.setLayoutData(gd);
      linkUp.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            movePort(type, true);
         }
      });

      final ImageHyperlink linkDown = toolkit.createImageHyperlink(clientArea, SWT.NONE);
      linkDown.setText("Down");
      linkDown.setImage(SharedIcons.IMG_DOWN);
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      linkDown.setLayoutData(gd);
      linkDown.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            movePort(type, false);
         }
      });
   }

   /* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		form.setFocus();
	}
	
	/**
	 * @param config
	 */
	private void setConfig(NetworkConfig config)
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
      snmpUsmCredList.setInput(config.getUsmCredentials(zoneUIN));
      getPortList(NetworkConfig.SNMP_PORTS).setInput(config.getPorts(NetworkConfig.SNMP_PORTS, zoneUIN));
      sharedSecretList.setInput(config.getSharedSecrets(zoneUIN));
      getPortList(NetworkConfig.AGENT_PORTS).setInput(config.getPorts(NetworkConfig.AGENT_PORTS, zoneUIN));
      sshCredList.setInput(config.getSshCredentials(zoneUIN));
      getPortList(NetworkConfig.SSH_PORTS).setInput(config.getPorts(NetworkConfig.SSH_PORTS, zoneUIN));
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

	/* (non-Javadoc)
	 * @see org.eclipse.ui.ISaveablePart#doSave(org.eclipse.core.runtime.IProgressMonitor)
	 */
	@Override
	public void doSave(IProgressMonitor monitor)
	{
		try
		{
			config.save(session);
		}
		catch(Exception e)
		{
			MessageDialogHelper.openError(getSite().getShell(), Messages.get().SnmpConfigurator_Error, 
			      String.format(Messages.get().NetworkCredentials_CannotSaveConfig, e.getLocalizedMessage()));
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
	 * Save settings
	 */
	private void save()
	{
	   if (bothModified && modified)
      {
         if (!MessageDialogHelper.openQuestion(getSite().getShell(), "Save network credential",
               "Network credentials already are modified by other users. Do you really want to continue and overwrite other users changes?\n"))
            return;
         content.hideMessage();   
      }	  

      saveInProgress = true;
		new ConsoleJob(Messages.get().NetworkCredentials_SaveConfig, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				config.save(session);
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
				return Messages.get().NetworkCredentials_ErrorSavingConfig;
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
         setModified(NetworkConfig.COMMUNITIES);
		}
	}
	
	/**
	 * Remove selected SNMP communities
	 */
	private void removeCommunity()
	{
		final List<String> list = config.getCommunities(zoneUIN);
		IStructuredSelection selection = (IStructuredSelection)snmpCommunityList.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
			{
				list.remove(o);
			}
			snmpCommunityList.setInput(list);
			setModified(NetworkConfig.COMMUNITIES);
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
      IStructuredSelection selection = (IStructuredSelection)snmpCommunityList.getSelection();
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
         setModified(NetworkConfig.COMMUNITIES);
      }
   }

	/**
	 * Add SNMP USM credentials to the list
	 */
	private void addUsmCredentials()
	{
		AddUsmCredDialog dlg = new AddUsmCredDialog(getSite().getShell(), null);
		if (dlg.open() == Window.OK)
		{
			SnmpUsmCredential cred = dlg.getValue();
			cred.setZoneId((int)zoneUIN);
         config.addUsmCredentials(cred, zoneUIN);
         snmpUsmCredList.setInput(config.getUsmCredentials(zoneUIN));
         setModified(NetworkConfig.USM);
		}
	}
   
   /**
    * Edit SNMP USM credential
    */
   private void editUsmCredentials()
   {
      IStructuredSelection selection = (IStructuredSelection)snmpUsmCredList.getSelection();
      if (selection.size() != 1)
         return;
      SnmpUsmCredential cred = (SnmpUsmCredential)selection.getFirstElement();
      AddUsmCredDialog dlg = new AddUsmCredDialog(getSite().getShell(), cred);
      if (dlg.open() == Window.OK)
      {
         final List<SnmpUsmCredential> list = config.getUsmCredentials(zoneUIN);
         snmpUsmCredList.setInput(list.toArray());
         setModified(NetworkConfig.USM);
      }
   }
	
	/**
	 * Remove selected SNMP USM credentials
	 */
	private void removeUsmCredentials()
	{
		final List<SnmpUsmCredential> list = config.getUsmCredentials(zoneUIN);
		IStructuredSelection selection = (IStructuredSelection)snmpUsmCredList.getSelection();
		if (selection.size() > 0)
		{
			for(Object o : selection.toList())
			{
				list.remove(o);
			}
			snmpUsmCredList.setInput(list.toArray());
			setModified(NetworkConfig.USM);
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
      IStructuredSelection selection = (IStructuredSelection)snmpUsmCredList.getSelection();
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
         snmpUsmCredList.setInput(list);
         setModified(NetworkConfig.USM);
      }
   }
	
   /**
    * Add agent shared secret
    */
   protected void addSharedSecret()
   {
      InputDialog dlg = new InputDialog(getSite().getShell(), "Add shared secret", 
            "Please enter shared secret", "", null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         config.addSharedSecret(value, zoneUIN);
         sharedSecretList.setInput(config.getSharedSecrets(zoneUIN));
         setModified(NetworkConfig.AGENT_SECRETS);
      }
   }

   /**
    * Remove agent shared secret
    */
   protected void removeSharedSecret()
   {
      final List<String> list = config.getSharedSecrets(zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)sharedSecretList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         sharedSecretList.setInput(list.toArray());
         setModified(NetworkConfig.AGENT_SECRETS);
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
      IStructuredSelection selection = (IStructuredSelection)sharedSecretList.getSelection();
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
         setModified(NetworkConfig.AGENT_SECRETS);
      }
   }

   /**
    * Add SSH credentials to the list
    */
   private void addSshCredentials()
   {
      AddSshCredDialog dlg = new AddSshCredDialog(getSite().getShell(), null, keyList);
      if (dlg.open() == Window.OK)
      {
         SshCredential cred = dlg.getValue();
         config.addSshCredentials(cred, zoneUIN);
         sshCredList.setInput(config.getSshCredentials(zoneUIN));
         setModified(NetworkConfig.SSH_CREDENTIALS);
      }
   }

   /**
    * Edit SSH credential
    */
   private void editSshCredentials()
   {
      IStructuredSelection selection = (IStructuredSelection)sshCredList.getSelection();
      if (selection.size() != 1)
         return;
      SshCredential cred = (SshCredential)selection.getFirstElement();
      AddSshCredDialog dlg = new AddSshCredDialog(getSite().getShell(), cred, keyList);
      if (dlg.open() == Window.OK)
      {
         final List<SshCredential> list = config.getSshCredentials(zoneUIN);
         sshCredList.setInput(list.toArray());
         setModified(NetworkConfig.SSH_CREDENTIALS);
      }
   }

   /**
    * Remove selected SSH credentials
    */
   private void removeSshCredentials()
   {
      final List<SshCredential> list = config.getSshCredentials(zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)sshCredList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove(o);
         }
         sshCredList.setInput(list.toArray());
         setModified(NetworkConfig.SSH_CREDENTIALS);
      }
   }

   /**
    * Move SSH credential
    * 
    * @param up true if up, false if down
    */
   protected void moveSshCredentials(boolean up)
   {
      final List<SshCredential> list = config.getSshCredentials(zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)sshCredList.getSelection();
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
         sshCredList.setInput(list);
         setModified(NetworkConfig.SSH_CREDENTIALS);
      }
   }

   /**
    * Add port to the list
    * 
    * @param type port type
    */
   private void addPort(int type)
   {
      InputDialog dlg = new InputDialog(getSite().getShell(), "Add " + getPortTypeAsString(type) + " port", 
            "Please enter " + getPortTypeAsString(type) + " port", "", null); //$NON-NLS-3$
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         config.addPort(type, Integer.parseInt(value), zoneUIN);
         getPortList(type).setInput(config.getPorts(type, zoneUIN));
         setModified(type);
      }
   }

   /**
    * Remove selected port
    * 
    * @param type port type
    */
   private void removePort(int type)
   {
      final List<Integer> list = config.getPorts(type, zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)getPortList(type).getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove((Integer)o);
         }
         getPortList(type).setInput(list.toArray());
         setModified(type);
      }
   }

   /**
    * Move port
    * 
    * @param type port type
    * @param up true if up, false if down
    */
   protected void movePort(int type, boolean up)
   {
      final List<Integer> list = config.getPorts(type, zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)getPortList(type).getSelection();
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
         getPortList(type).setInput(list);
         setModified(type);
      }
   }
}
