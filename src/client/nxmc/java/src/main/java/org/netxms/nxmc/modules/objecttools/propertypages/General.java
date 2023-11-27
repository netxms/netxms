/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objecttools.propertypages;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.FileDialog;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.client.objecttools.ObjectToolDetails;
import org.netxms.nxmc.base.propertypages.PropertyPage;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for object tool
 */
public class General extends PropertyPage
{
   private final I18n i18n = LocalizationHelper.getI18n(General.class);
   private static final Logger logger = LoggerFactory.getLogger(General.class);

	private ObjectToolDetails objectTool;
	private LabeledText textName;
	private LabeledText textDescription;
	private LabeledText textData;
	private LabeledSpinner maxFileSize;
	private LabeledText textParameter;
	private LabeledText textRegexp;
	private Button checkShowOutput;
   private Button checkSuppressSuccessMessage;
   private Button checkSetupTCPTunnel;
   private Spinner remotePort;
	private Button checkConfirmation;
   private LabeledText textConfirmation;
	private Button checkDisable;
   private Button checkRunInContainerContext;
	private Button checkFollow;
	private Button checkCommand;
	private LabeledText textCommandName;
   private LabeledText textCommandShortName;
	private Button radioIndexOID;
	private Button radioIndexValue;
	private Label iconLabel;
	private Image icon;

	/**
	 * Constructor
	 * 
	 * @param toolDetails
	 */
	public General(ObjectToolDetails toolDetails)
	{
      super("General");
      noDefaultAndApplyButton();
	   objectTool = toolDetails;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{			
      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);

      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(i18n.tr("Name"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textName.setLayoutData(gd);
      textName.setText(objectTool.getName());

      createIcon();
      createIconSelector(dialogArea);

		textDescription = new LabeledText(dialogArea, SWT.NONE);
		textDescription.setLabel(i18n.tr("Description"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		textDescription.setLayoutData(gd);
		textDescription.setText(objectTool.getDescription());

		textData = new LabeledText(dialogArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		textData.setLayoutData(gd);
		textData.setText(objectTool.getData());

		switch(objectTool.getToolType())
		{
			case ObjectTool.TYPE_INTERNAL:
				textData.setLabel(i18n.tr("Operation"));
				break;
			case ObjectTool.TYPE_LOCAL_COMMAND:
				textData.setLabel(i18n.tr("Command"));
            createTCPTunnelGroup(dialogArea);
            createOutputGroup(dialogArea);
            break;
			case ObjectTool.TYPE_SERVER_COMMAND:
			case ObjectTool.TYPE_SSH_COMMAND:
				textData.setLabel(i18n.tr("Command"));
				createOutputGroup(dialogArea);
				break;
         case ObjectTool.TYPE_SERVER_SCRIPT:
            textData.setLabel(i18n.tr("Script"));
            createOutputGroup(dialogArea);
            break;
			case ObjectTool.TYPE_ACTION:
				textData.setLabel(i18n.tr("Agent's command"));
				createOutputGroup(dialogArea);
				break;
			case ObjectTool.TYPE_URL:
				textData.setLabel(i18n.tr("URL"));
            createTCPTunnelGroup(dialogArea);
				break;
			case ObjectTool.TYPE_FILE_DOWNLOAD:
			   String[] parameters = objectTool.getData().split("\u007F"); //$NON-NLS-1$

				textData.setLabel(i18n.tr("Remote file name"));
				textData.setText((parameters.length > 0) ? parameters[0] : ""); //$NON-NLS-1$

				Group fileOptionsGoup = new Group(dialogArea, SWT.NONE);
				fileOptionsGoup.setText(i18n.tr("File Options"));
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            gd.horizontalSpan = 2;
            fileOptionsGoup.setLayoutData(gd);

            GridLayout fileGroupLayout = new GridLayout();
				fileGroupLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
				fileGroupLayout.numColumns = 1;
				fileOptionsGoup.setLayout(fileGroupLayout);

		      maxFileSize  = new LabeledSpinner(fileOptionsGoup, SWT.NONE);
		      gd = new GridData();
		      gd.horizontalAlignment = SWT.FILL;
		      gd.grabExcessHorizontalSpace = true;
		      maxFileSize.setLayoutData(gd);
				maxFileSize.setLabel(i18n.tr("Limit initial download size (in bytes, 0 for unlimited)"));
				maxFileSize.setRange(0, 0x7FFFFFFF);
				try
				{
				   maxFileSize.setSelection((parameters.length > 1) ? Integer.parseInt(parameters[1]) : 0);
				}
				catch(NumberFormatException e)
				{
				   maxFileSize.setSelection(0);
				}
							
				checkFollow = new Button(fileOptionsGoup, SWT.CHECK);
				checkFollow.setText(i18n.tr("Follow file changes"));
				if(parameters.length > 2) //$NON-NLS-1$
				{
				   checkFollow.setSelection(parameters[2].equals("true"));  //$NON-NLS-1$
				}	
				break;
			case ObjectTool.TYPE_SNMP_TABLE:
				textData.setLabel(i18n.tr("Title"));
				
				Group snmpOptGroup = new Group(dialogArea, SWT.NONE);
				snmpOptGroup.setText(i18n.tr("SNMP List Options"));
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalSpan = 2;
				snmpOptGroup.setLayoutData(gd);
				layout = new GridLayout();
				snmpOptGroup.setLayout(layout);

				new Label(snmpOptGroup, SWT.NONE).setText(i18n.tr("Use as index for second and subsequent columns:"));

				radioIndexOID = new Button(snmpOptGroup, SWT.RADIO);
				radioIndexOID.setText(i18n.tr("&OID suffix of first column"));
				radioIndexOID.setSelection((objectTool.getFlags() & ObjectTool.SNMP_INDEXED_BY_VALUE) == 0);
				
				radioIndexValue = new Button(snmpOptGroup, SWT.RADIO);
				radioIndexValue.setText(i18n.tr("&Value of first column"));
				radioIndexValue.setSelection(!radioIndexOID.getSelection());

				break;
			case ObjectTool.TYPE_AGENT_LIST:
				textData.setLabel(i18n.tr("Title"));
				
				String[] listParts = objectTool.getData().split("\u007F"); //$NON-NLS-1$
				textData.setText((listParts.length > 0) ? listParts[0] : ""); //$NON-NLS-1$

				textParameter = new LabeledText(dialogArea, SWT.NONE);
				textParameter.setLabel(i18n.tr("Parameter"));
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalSpan = 2;
				textParameter.setLayoutData(gd);
				textParameter.setText((listParts.length > 1) ? listParts[1] : ""); //$NON-NLS-1$

				textRegexp = new LabeledText(dialogArea, SWT.NONE);
				textRegexp.setLabel(i18n.tr("Regular expression"));
				gd = new GridData();
				gd.horizontalAlignment = SWT.FILL;
				gd.grabExcessHorizontalSpace = true;
            gd.horizontalSpan = 2;
				textRegexp.setLayoutData(gd);
				textRegexp.setText((listParts.length > 2) ? listParts[2] : ""); //$NON-NLS-1$
				break;
         case ObjectTool.TYPE_AGENT_TABLE:
            textData.setLabel(i18n.tr("Title"));
            
            String[] tableParts = objectTool.getData().split("\u007F"); //$NON-NLS-1$
            textData.setText((tableParts.length > 0) ? tableParts[0] : ""); //$NON-NLS-1$

            textParameter = new LabeledText(dialogArea, SWT.NONE);
            textParameter.setLabel(i18n.tr("Parameter"));
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            gd.horizontalSpan = 2;
            textParameter.setLayoutData(gd);
            textParameter.setText((tableParts.length > 1) ? tableParts[1] : ""); //$NON-NLS-1$
            break;
		}

		Group confirmationGroup = new Group(dialogArea, SWT.NONE);
		confirmationGroup.setText(i18n.tr("Confirmation"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		confirmationGroup.setLayoutData(gd);
		layout = new GridLayout();
		confirmationGroup.setLayout(layout);

		checkConfirmation = new Button(confirmationGroup, SWT.CHECK);
		checkConfirmation.setText(i18n.tr("This tool requires confirmation before execution"));
		checkConfirmation.setSelection((objectTool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0);
      checkConfirmation.addSelectionListener(new SelectionAdapter() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				textConfirmation.setEnabled(checkConfirmation.getSelection());
				if (checkConfirmation.getSelection())
					textConfirmation.setFocus();
			}
		});

		textConfirmation = new LabeledText(confirmationGroup, SWT.NONE);
		textConfirmation.setLabel(i18n.tr("Confirmation message"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		textConfirmation.setLayoutData(gd);
		textConfirmation.setText(objectTool.getConfirmationText());
		textConfirmation.setEnabled(checkConfirmation.getSelection());

      Group commandGroup = new Group(dialogArea, SWT.NONE);
      commandGroup.setText(i18n.tr("Show in commands"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      commandGroup.setLayoutData(gd);
      layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      commandGroup.setLayout(layout);

      checkCommand = new Button(commandGroup, SWT.CHECK);
      checkCommand.setText(i18n.tr("Show this tool in node commands"));
      checkCommand.setSelection((objectTool.getFlags() & ObjectTool.SHOW_IN_COMMANDS) != 0);
      checkCommand.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            textCommandName.setEnabled(checkCommand.getSelection());
            textCommandShortName.setEnabled(checkCommand.getSelection());
            if (checkCommand.getSelection())
               textCommandName.setFocus();
         }
      });
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      checkCommand.setLayoutData(gd);

      textCommandName = new LabeledText(commandGroup, SWT.NONE);
      textCommandName.setLabel(i18n.tr("Command name"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textCommandName.setLayoutData(gd);
      textCommandName.setText(objectTool.getCommandName());
      textCommandName.setEnabled(checkCommand.getSelection());
		
      textCommandShortName = new LabeledText(commandGroup, SWT.NONE);
      textCommandShortName.setLabel(i18n.tr("Command short name"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textCommandShortName.setLayoutData(gd);
      textCommandShortName.setText(objectTool.getCommandShortName());
      textCommandShortName.setEnabled(checkCommand.getSelection());

      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Other options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      optionsGroup.setLayoutData(gd);
      optionsGroup.setLayout(new GridLayout());

      // Disable option
      checkDisable = new Button(optionsGroup, SWT.CHECK);
      gd = new GridData();
      gd.horizontalSpan = 2;
      checkDisable.setLayoutData(gd);
      checkDisable.setText(i18n.tr("&Disabled"));
      checkDisable.setSelection(!objectTool.isEnabled());

      // Run in container context
      if ((objectTool.getToolType() == ObjectTool.TYPE_LOCAL_COMMAND) || (objectTool.getToolType() == ObjectTool.TYPE_SERVER_COMMAND) || (objectTool.getToolType() == ObjectTool.TYPE_SERVER_SCRIPT) ||
            (objectTool.getToolType() == ObjectTool.TYPE_URL))
      {
         checkRunInContainerContext = new Button(optionsGroup, SWT.CHECK);
         gd = new GridData();
         gd.horizontalSpan = 2;
         checkDisable.setLayoutData(gd);
         checkRunInContainerContext.setText(i18n.tr("Run in &container context"));
         checkRunInContainerContext.setSelection(objectTool.isRunInContainerContext());
      }

		return dialogArea;
	}
	
	/**
	 * @param parent
	 */
	private void createOutputGroup(Composite parent)
	{
		Group outputGroup = new Group(parent, SWT.NONE);
		outputGroup.setText(i18n.tr("Execution options"));
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
		outputGroup.setLayoutData(gd);
		outputGroup.setLayout(new GridLayout());

		checkShowOutput = new Button(outputGroup, SWT.CHECK);
		checkShowOutput.setText(i18n.tr("Command generates output"));
		checkShowOutput.setSelection((objectTool.getFlags() & ObjectTool.GENERATES_OUTPUT) != 0);

      checkSuppressSuccessMessage = new Button(outputGroup, SWT.CHECK);
      checkSuppressSuccessMessage.setText("&Suppress notification of successful execution");
      checkSuppressSuccessMessage.setSelection((objectTool.getFlags() & ObjectTool.SUPPRESS_SUCCESS_MESSAGE) != 0);
	}

   /**
    * @param parent
    */
   private void createTCPTunnelGroup(Composite parent)
   {
      Group tcpTunnelGroup = new Group(parent, SWT.NONE);
      tcpTunnelGroup.setText(i18n.tr("TCP tunnel"));
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      tcpTunnelGroup.setLayoutData(gd);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      tcpTunnelGroup.setLayout(layout);

      checkSetupTCPTunnel = new Button(tcpTunnelGroup, SWT.CHECK);
      checkSetupTCPTunnel.setText(i18n.tr("Setup &TCP tunnel to remote port"));
      checkSetupTCPTunnel.setSelection((objectTool.getFlags() & ObjectTool.SETUP_TCP_TUNNEL) != 0);
      checkSetupTCPTunnel.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      checkSetupTCPTunnel.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            remotePort.setEnabled(checkSetupTCPTunnel.getSelection());
         }
      });

      remotePort = new Spinner(tcpTunnelGroup, SWT.BORDER);
      remotePort.setMinimum(1);
      remotePort.setMaximum(65535);
      remotePort.setSelection(objectTool.getRemotePort());
      remotePort.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false));
      remotePort.setEnabled(checkSetupTCPTunnel.getSelection());
   }

	/**
	 * Create icon
	 */
   @SuppressWarnings("deprecation")
   private void createIcon()
	{
	   if (icon != null)
	   {
	      icon.dispose();
	      icon = null;
	   }

      byte[] imageBytes = objectTool.getImageData();
      if ((imageBytes == null) || (imageBytes.length == 0))
         return;

      ByteArrayInputStream input = new ByteArrayInputStream(imageBytes);
      try
      {
         ImageDescriptor d = ImageDescriptor.createFromImageData(new ImageData(input));
         icon = d.createImage();
      }
      catch(Exception e)
      {
         logger.error("Exception in ObjectToolsGeneral.createIcon()", e);
      }
	}
	
	/**
	 * @param parent
	 */
	private void createIconSelector(Composite parent)
	{
	   Group group = new Group(parent, SWT.NONE);
	   group.setText(i18n.tr("Icon"));
	   group.setLayoutData(new GridData(SWT.FILL, SWT.FILL, false, false));
	   
	   GridLayout layout = new GridLayout();
	   layout.numColumns = 4;
	   group.setLayout(layout);
	   
	   iconLabel = new Label(group, SWT.NONE);
      iconLabel.setImage((icon != null) ? icon : SharedIcons.IMG_EMPTY);
      iconLabel.setLayoutData(new GridData(SWT.LEFT, SWT.CENTER, false, true));
      
      Label dummy = new Label(group, SWT.NONE);
      GridData gd = new GridData();
      gd.widthHint = 8;
      dummy.setLayoutData(gd);
	   
      Button link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_FIND);
      link.setToolTipText(i18n.tr("Select..."));
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            selectIcon();
         }
      });

      link = new Button(group, SWT.PUSH);
      link.setImage(SharedIcons.IMG_CLEAR);
      link.setToolTipText(i18n.tr("Clear"));
      link.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            iconLabel.setImage(SharedIcons.IMG_EMPTY);
            if (icon != null)
            {
               icon.dispose();
               icon = null;
            }
         }
      });
      link.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            if (icon != null)
               icon.dispose();
         }
      });
	}
	
	/**
	 * 
	 */
	private void selectIcon()
	{
	   FileDialog dlg = new FileDialog(getShell(), SWT.OPEN);
	   WidgetHelper.setFileDialogFilterNames(dlg, new String[] { i18n.tr("Image Files"), i18n.tr("All Files") });
	   WidgetHelper.setFileDialogFilterExtensions(dlg, new String[] { "*.gif;*.jpg;*.png", "*.*" }); 

      String fileName = dlg.open();
      if (fileName == null)
         return;
      
      try
      {
         Image image = new Image(getShell().getDisplay(), new FileInputStream(new File(fileName)));
         if ((image.getImageData().width <= 16) && (image.getImageData().height <= 16))
         {
            if (icon != null)
               icon.dispose();
            icon = image;
            iconLabel.setImage(icon);
         }
         else
         {
            image.dispose();
            MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Select image file is too large"));
         }
      }
      catch(Exception e)
      {
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), String.format(i18n.tr("Cannot load image file: %s"), e.getLocalizedMessage()));
      }
	}

   /**
    * @see org.netxms.nxmc.base.propertypages.PropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(boolean isApply)
	{	   
		objectTool.setName(textName.getText());
		objectTool.setDescription(textDescription.getText());

		// Tool data
		switch(objectTool.getToolType())
		{
		   case ObjectTool.TYPE_AGENT_LIST:
	         objectTool.setData(textData.getText() + "\u007F" + textParameter.getText() + "\u007F" + textRegexp.getText()); //$NON-NLS-1$ //$NON-NLS-2$
	         break;
         case ObjectTool.TYPE_AGENT_TABLE:
            objectTool.setData(textData.getText() + "\u007F" + textParameter.getText()); //$NON-NLS-1$
            break;
		   case ObjectTool.TYPE_FILE_DOWNLOAD:
            objectTool.setData(textData.getText() + "\u007F" + maxFileSize.getSelection() + "\u007F" + checkFollow.getSelection()); //$NON-NLS-1$ //$NON-NLS-2$
		      break;
		   default:
            objectTool.setData(textData.getText());
            break;
		}
		
		if (checkConfirmation.getSelection())
		{
			objectTool.setFlags(objectTool.getFlags() | ObjectTool.ASK_CONFIRMATION);
		}
		else
		{
			objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.ASK_CONFIRMATION);
		}
		objectTool.setConfirmationText(textConfirmation.getText());
		
      if (checkCommand.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.SHOW_IN_COMMANDS);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.SHOW_IN_COMMANDS);
      }
      objectTool.setCommandName(textCommandName.getText());
      objectTool.setCommandShortName(textCommandShortName.getText());
      
      if (checkDisable.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.DISABLED);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.DISABLED);
      }

      if ((checkRunInContainerContext != null) && checkRunInContainerContext.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.RUN_IN_CONTAINER_CONTEXT);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.RUN_IN_CONTAINER_CONTEXT);
      }

		if (objectTool.getToolType() == ObjectTool.TYPE_SNMP_TABLE)
		{
			if (radioIndexValue.getSelection())
			{
				objectTool.setFlags(objectTool.getFlags() | ObjectTool.SNMP_INDEXED_BY_VALUE);
			}
			else
			{
				objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.SNMP_INDEXED_BY_VALUE);
			}
		}

      if ((checkShowOutput != null) && checkShowOutput.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.GENERATES_OUTPUT);
      }
      else
		{
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.GENERATES_OUTPUT);
		}

      if ((checkSuppressSuccessMessage != null) && checkSuppressSuccessMessage.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.SUPPRESS_SUCCESS_MESSAGE);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.SUPPRESS_SUCCESS_MESSAGE);
      }

      if ((checkSetupTCPTunnel != null) && checkSetupTCPTunnel.getSelection())
      {
         objectTool.setFlags(objectTool.getFlags() | ObjectTool.SETUP_TCP_TUNNEL);
      }
      else
      {
         objectTool.setFlags(objectTool.getFlags() & ~ObjectTool.SETUP_TCP_TUNNEL);
      }

      if (remotePort != null)
      {
         objectTool.setRemotePort(remotePort.getSelection());
      }

		if (icon != null)
		{
		   ImageLoader loader = new ImageLoader();
		   loader.data = new ImageData[] { icon.getImageData() };
		   ByteArrayOutputStream stream = new ByteArrayOutputStream(1024);
		   loader.save(stream, SWT.IMAGE_PNG);
		   objectTool.setImageData(stream.toByteArray());
		}
		else
		{
		   objectTool.setImageData(null);
		}
		return true;
	}
}
