/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2014 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.objects.AgentPolicy;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object creation dialog
 *
 */
public class CreatePolicyDialog extends Dialog
{
	private String policyName;
   private String policyType;
	private Text textName;
	private Combo typeSelector;
	private AgentPolicy policy;

	/**
	 * Constructor
	 * 
	 * @param parentShell Parent shell
	 * @param policy 
	 * @param objectClassName Object class - this string will be added to dialog's title
	 */
	public CreatePolicyDialog(Shell parentShell, AgentPolicy policy)
	{
		super(parentShell);
		this.policy = policy;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Create new policy");
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
		
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, SWT.DEFAULT,
      		"Policy name", "", WidgetHelper.DEFAULT_LAYOUT_DATA); //$NON-NLS-1$
      textName.getShell().setMinimumSize(300, 0);
      textName.setTextLimit(63);
      textName.setFocus();
      
      if(policy != null)
      {
         textName.setText(policy.getName());
      }
      else
      {         
         typeSelector = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, "Policy type",
               WidgetHelper.DEFAULT_LAYOUT_DATA);
         typeSelector.add(AgentPolicy.agent);
         typeSelector.add(AgentPolicy.logParser);
         typeSelector.add(AgentPolicy.supportApplication);
         typeSelector.select(0);
      }
      
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		policyName = textName.getText().trim();
      if(policy == null)
      {
         policyType = typeSelector.getText();
      }
		if (policyName.isEmpty())
		{
			MessageDialogHelper.openWarning(getShell(), "Warning", "Policy name can not be empty");
			return;
		}
		super.okPressed();
	}

   public AgentPolicy getPolicy()
   {
      if(policy == null)
         return new AgentPolicy(policyName, policyType);
      
      policy.setName(policyName);
      return policy;
   }
}
