/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.serverconfig.views;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.dialogs.InputDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
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
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.SSHCredentials;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.SshKeyPair;
import org.netxms.client.snmp.SnmpUsmCredential;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.widgets.ZoneSelector;
import org.netxms.nxmc.modules.serverconfig.NetworkCredentials;
import org.netxms.nxmc.modules.serverconfig.dialogs.EditSSHCredentialsDialog;
import org.netxms.nxmc.modules.serverconfig.dialogs.EditSnmpUsmCredentialsDialog;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SnmpUsmCredentialsLabelProvider;
import org.netxms.nxmc.modules.serverconfig.views.helpers.SshCredentialsLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Configurator for network discovery
 */
public class NetworkCredentialsEditor extends ConfigurationView
{
   private static final I18n i18n = LocalizationHelper.getI18n(NetworkCredentialsEditor.class);

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
   private int warningMessageId = 0;
   private Composite content;
	private TableViewer snmpCommunityList;
	private SortableTableViewer snmpUsmCredentialsList;
   private TableViewer snmpPortList;
   private TableViewer sharedSecretList;
   private TableViewer agentPortList;
   private TableViewer sshCredentialsList;
   private TableViewer sshPortList;
	private Action actionSave;
	private NetworkCredentials config;
	private ZoneSelector zoneSelector;
   private Display display;
   private int zoneUIN = NetworkCredentials.NETWORK_CONFIG_GLOBAL;
   private List<SshKeyPair> sshKeys;
   private SshCredentialsLabelProvider sshLabelProvider;

   /**
    * Create notification channels view
    */
   public NetworkCredentialsEditor()
   {
      super(i18n.tr("Network Credentials"), ResourceManager.getImageDescriptor("icons/config-views/network_credentials.png"), "NetworkCredentials", false);
      session = Registry.getSession();
      config = new NetworkCredentials(session);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
	{
      display = parent.getDisplay();

      ScrolledComposite scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      scroller.getVerticalBar().setIncrement(20);
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
                        if (warningMessageId == 0)
                        {
                           warningMessageId = addMessage(MessageArea.WARNING,
                                 i18n.tr("Network credentials are modified by other users. \"Refresh\" will discard local changes. \"Save\" will overwrite other users changes with local changes."));
                        }
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
      new Job(i18n.tr("Loading network credentials"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<SshKeyPair> sshKeys = session.getSshKeys(false);
            config.load(configId, zoneUIN);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  if (isClientAreaDisposed())
                     return;
                  NetworkCredentialsEditor.this.sshKeys = sshKeys;
                  sshLabelProvider.setKeyList(sshKeys);
                  setConfig(config);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load network credentials");
         }
      }.start();
	}

	/**
	 * Create actions
	 */
	private void createActions()
	{
      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
			@Override
			public void run()
			{
            if (!modified)
               return;

            if (bothModified)
            {
               if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Save Network Credentials"),
                     i18n.tr(i18n.tr("Network credentials are modified by you and other users. Do you want to save your changes and overwrite other users' changes?"))))
                  return;
            }

				save();
			}
		};
      addKeyBinding("M1+S", actionSave);
	}

	/**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
	{
      if (modified)
      {
         if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Refresh Network Credentials"),
               i18n.tr("This will discard all unsaved changes. Do you really want to continue?")))
            return;          
      }
      loadConfiguration(NetworkCredentials.EVERYTHING, NetworkCredentials.ALL_ZONES);  

      modified = false;
      bothModified = false;
	}

	/**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolbar(org.eclipse.jface.action.ToolBarManager)
    */
   @Override
   protected void fillLocalToolbar(ToolBarManager manager)
   {
      manager.add(actionSave);
   }

   /**
    * Create "SNMP Communities" section
    */
	private void createSnmpCommunitySection()
	{
      Group clientArea = new Group(content, SWT.NONE);
      clientArea.setText(i18n.tr("SNMP community strings"));
      clientArea.setBackground(content.getBackground());
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		clientArea.setLayout(layout);

		snmpCommunityList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 4;
		gd.heightHint = 150;
		snmpCommunityList.getTable().setLayoutData(gd);
		snmpCommunityList.setContentProvider(new ArrayContentProvider());

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
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

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
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

      final ImageHyperlink linkUp = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkDown = new ImageHyperlink(clientArea, SWT.NONE);
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
	}

   /**
	 * Create "Address Filters" section
	 */
	private void createSnmpUsmCredSection()
	{
      Group clientArea = new Group(content, SWT.NONE);
      clientArea.setText(i18n.tr("SNMPv3 USM credentials"));
      clientArea.setBackground(content.getBackground());
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 3, 1));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);
		
      final String[] names = { "User name", "Auth type", "Priv type", "Auth password", "Priv password", "Comments" };
		final int[] widths = { 100, 100, 100, 100, 100, 100 };
		snmpUsmCredentialsList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 5;
		gd.heightHint = 150;
		snmpUsmCredentialsList.getTable().setLayoutData(gd);
		snmpUsmCredentialsList.setContentProvider(new ArrayContentProvider());
		snmpUsmCredentialsList.setLabelProvider(new SnmpUsmCredentialsLabelProvider());
		snmpUsmCredentialsList.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            editUsmCredentials();
         }
      });

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
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

      final ImageHyperlink linkEdit = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
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

      final ImageHyperlink linkUp = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkDown = new ImageHyperlink(clientArea, SWT.NONE);
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
	}

   /**
    * Create "Agent shared secrets" section
    */
   private void createSharedSecretList()
   {
      Group clientArea = new Group(content, SWT.NONE);
      clientArea.setText("Agent shared secrets");
      clientArea.setBackground(content.getBackground());
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      sharedSecretList = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 150;
      sharedSecretList.getTable().setLayoutData(gd);
      sharedSecretList.setContentProvider(new ArrayContentProvider());

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
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
      
      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
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

      final ImageHyperlink linkUp = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkDown = new ImageHyperlink(clientArea, SWT.NONE);
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
   }

   /**
    * Create "SSH Credentials" section
    */
   private void createSshCredentialsList()
   {
      Group clientArea = new Group(content, SWT.NONE);
      clientArea.setText("SSH credentials");
      clientArea.setBackground(content.getBackground());
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false, 2, 1));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      final String[] names = { "Login", "Password", "Key" };
      final int[] widths = { 150, 150, 150 };
      sshCredentialsList = new SortableTableViewer(clientArea, names, widths, 0, SWT.DOWN, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 5;
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

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
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

      final ImageHyperlink linkEdit = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
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

      final ImageHyperlink linkUp = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkDown = new ImageHyperlink(clientArea, SWT.NONE);
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
   }

   /**
    * Create "XXX ports" section
    */
   private TableViewer createPortList(int portType, String typeName)
   {
      Group clientArea = new Group(content, SWT.NONE);
      clientArea.setText(String.format("%s ports", typeName));
      clientArea.setBackground(content.getBackground());
      clientArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      clientArea.setLayout(layout);

      final TableViewer viewer = new TableViewer(clientArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
      gd.heightHint = 150;
      viewer.getTable().setLayoutData(gd);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setData("PortType", Integer.valueOf(portType));

      final ImageHyperlink linkAdd = new ImageHyperlink(clientArea, SWT.NONE);
      linkAdd.setText(i18n.tr("Add"));
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

      final ImageHyperlink linkRemove = new ImageHyperlink(clientArea, SWT.NONE);
      linkRemove.setText(i18n.tr("Remove"));
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

      final ImageHyperlink linkUp = new ImageHyperlink(clientArea, SWT.NONE);
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

      final ImageHyperlink linkDown = new ImageHyperlink(clientArea, SWT.NONE);
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
		}
	}

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#getSaveOnExitPrompt()
    */
   @Override
   public String getSaveOnExitPrompt()
   {
      return bothModified ?
            i18n.tr("Network credentials are modified by you and other users. Do you want to save your changes and overwrite other users' changes?") :
            i18n.tr("Network credentials are modified. Do you want to save your changes?");
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
	{
      saveInProgress = true;
      new Job(i18n.tr("Updating network credentials"), this) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
            config.save();
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						modified = false;
			         bothModified = false;
                  saveInProgress = false;
                  deleteMessage(warningMessageId);
                  warningMessageId = 0;
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
            return i18n.tr("Cannot update network credentials");
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
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Add SNMP Community String"), 
            i18n.tr("Enter SNMP community string"), "", null);
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
      EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getWindow().getShell(), null);
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
      EditSnmpUsmCredentialsDialog dlg = new EditSnmpUsmCredentialsDialog(getWindow().getShell(), cred);
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
      InputDialog dlg = new InputDialog(getWindow().getShell(), i18n.tr("Add Shared Secret"), i18n.tr("Enter shared secret"), "", null);
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
      EditSSHCredentialsDialog dlg = new EditSSHCredentialsDialog(getWindow().getShell(), null, sshKeys);
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
      EditSSHCredentialsDialog dlg = new EditSSHCredentialsDialog(getWindow().getShell(), cred, sshKeys);
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
      InputDialog dlg = new InputDialog(getWindow().getShell(), String.format(i18n.tr("Add %s Port"), typeName), String.format(i18n.tr("Please enter %s port"), typeName), "", null);
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
}
