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
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ZoneSelector;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.snmp.Activator;
import org.netxms.ui.eclipse.snmp.Messages;
import org.netxms.ui.eclipse.snmp.dialogs.AddUsmCredDialog;
import org.netxms.ui.eclipse.snmp.views.helpers.NetworkConfig;
import org.netxms.ui.eclipse.snmp.views.helpers.SnmpUsmLabelProvider;
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
	private Action actionSave;
   private RefreshAction actionRefresh;
	private NetworkConfig config;
	private ZoneSelector zoneSelector;
   private Display display;
	private long zoneUIN = NetworkConfig.NETWORK_CONFIG_GLOBAL;


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
                  type = NetworkConfig.PORTS;
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
		createSnmpPortList();
      createSnmpUsmCredSection();
      createSharedSecretList();
		
		createActions();
		contributeToActionBars();

      // Load config
      loadSnmpConfig(NetworkConfig.ALL_CONFIGS, NetworkConfig.ALL_ZONES);
	}
	
	/**
	 * Load SNMP config
	 * @param zoneUIN of config
	 */
	private void loadSnmpConfig(final int configId, final int zoneUIN)
	{
	   new ConsoleJob(Messages.get().NetworkCredentials_LoadingConfig, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
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
	 * @param manager
	 *           Menu manager for local toolbar
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
            editUsmCredzantial();
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
            editUsmCredzantial();
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
    * Create "Port List" section
    */
   private void createSnmpPortList()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText(Messages.get().NetworkCredentials_Ports);
      section.setDescription(Messages.get().NetworkCredentials_PortsDescription);
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      Composite clientArea = toolkit.createComposite(section);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
      section.setClient(clientArea);
      
      snmpPortList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      toolkit.adapt(snmpPortList.getTable());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 150;
      snmpPortList.getTable().setLayoutData(gd);
      snmpPortList.setContentProvider(new ArrayContentProvider());

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
            addSnmpPort();
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
            removeSnmpPort();
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
            moveSnmpPort(true);
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
            moveSnmpPort(false);
         }
      });
   }

   /**
    * Create "Port List" section
    */
   private void createSharedSecretList()
   {
      Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
      section.setText("Shared secrets");
      section.setDescription("Shared secrets used in the network");
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
      snmpPortList.setInput(config.getPorts(zoneUIN));
      sharedSecretList.setInput(config.getSharedSecrets(zoneUIN));	   
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
   private void editUsmCredzantial()
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
    * Add SNMP port to the list
    */
   private void addSnmpPort()
   {
      InputDialog dlg = new InputDialog(getSite().getShell(), Messages.get().NetworkCredentials_AddPort, 
            Messages.get().NetworkCredentials_PleaseEnterPort, "", null); //$NON-NLS-1$
      if (dlg.open() == Window.OK)
      {
         String value = dlg.getValue();
         config.addPort(Integer.parseInt(value), zoneUIN);
         snmpPortList.setInput(config.getPorts(zoneUIN));
         setModified(NetworkConfig.PORTS);
      }
   }
   
   /**
    * Remove selected SNMP port
    */
   private void removeSnmpPort()
   {
      final List<Integer> list = config.getPorts(zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)snmpPortList.getSelection();
      if (selection.size() > 0)
      {
         for(Object o : selection.toList())
         {
            list.remove((Integer)o);
         }
         snmpPortList.setInput(list.toArray());
         setModified(NetworkConfig.PORTS);
      }
   }

   /**
    * Move SNMP port
    * 
    * @param up true if up, false if down
    */
   protected void moveSnmpPort(boolean up)
   {
      final List<Integer> list = config.getPorts(zoneUIN);
      IStructuredSelection selection = (IStructuredSelection)snmpPortList.getSelection();
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
         snmpPortList.setInput(list);
         setModified(NetworkConfig.PORTS);
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
}
