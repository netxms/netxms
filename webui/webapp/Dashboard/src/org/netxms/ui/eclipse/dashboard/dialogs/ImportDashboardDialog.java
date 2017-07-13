/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.dialogs;

import java.io.File;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.filemanager.widgets.LocalFileSelector;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;

/**
 * "Import Dashboard" dialog
 */
public class ImportDashboardDialog extends Dialog
{
	private String objectName;
	private File importFile;
	private LabeledText textName;
	private LocalFileSelector importFileSelector;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 */
	public ImportDashboardDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText(Messages.get().ImportDashboardDialog_Title);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);

		GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel(Messages.get().ImportDashboardDialog_ObjectName);
      textName.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      textName.setLayoutData(gd);
      
      importFileSelector = new LocalFileSelector(dialogArea, SWT.NONE, false, SWT.OPEN);
      importFileSelector.setLabel(Messages.get().ImportDashboardDialog_ImportFile);
      importFileSelector.setFilterExtensions(new String[] { "*.xml", "*.*" }); //$NON-NLS-1$ //$NON-NLS-2$
      importFileSelector.setFilterNames(new String[] { Messages.get().ImportDashboardDialog_XMLFiles, Messages.get().ImportDashboardDialog_AllFiles });
      importFileSelector.addModifyListener(new ModifyListener() {
         
         @Override
         public void modifyText(ModifyEvent e)
         {
            Element root = null;
            try
            {
               DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
               DocumentBuilder db = dbf.newDocumentBuilder();
               Document dom = db.parse(importFileSelector.getFile());
               
               root = dom.getDocumentElement();
               if (!root.getNodeName().equals("dashboard")) //$NON-NLS-1$
                  throw new Exception(Messages.get().ImportDashboard_InvalidFile);               

               root.normalize();
            }
            catch(Exception e1)
            {
               MessageDialogHelper.openError(getShell(), "Error", "Unable to open the document");
            }
            finally
            {
               if (root == null)
                  return;
               
               NodeList nameRoot = root.getElementsByTagName("name");
               textName.setText(nameRoot.item(0).getTextContent());
               
               root.normalize();
            }
        }
      });
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      importFileSelector.setLayoutData(gd);
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
	   objectName = textName.getText();
		importFile = importFileSelector.getFile();
		
		if (importFile == null)
		{
			MessageDialogHelper.openWarning(getShell(), Messages.get().ImportDashboardDialog_Warning, Messages.get().ImportDashboardDialog_WarningSelectFile);
			return;
		}
		
		super.okPressed();
	}

	/**
	 * @return the objectName
	 */
	public String getObjectName()
	{
		return objectName;
	}

	/**
	 * @return the importFile
	 */
	public File getImportFile()
	{
		return importFile;
	}
}
