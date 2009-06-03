/**
 * 
 */
package org.netxms.ui.eclipse.policymanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCAgentPolicyConfig;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class ConfigFile extends PropertyPage
{
	private Text textName;
	private Text textContent;
	private String initialName;
	private String initialContent;
	private NXCAgentPolicyConfig object;
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NXCAgentPolicyConfig)getElement().getAdapter(NXCAgentPolicyConfig.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		//FormLayout layout = new FormLayout();
      dialogArea.setLayout(layout);
      
		// File name
      initialName = new String(object.getFileName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, "File name",
      		                                    initialName, null);
		
		// File content
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("File");
      
      initialContent = new String(object.getFileContent());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = GridData.FILL;
      gd.verticalAlignment = GridData.FILL;
      gd.heightHint = 0;
      textContent = new Text(dialogArea, SWT.MULTI | SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
      textContent.setText(object.getFileContent());
      textContent.setLayoutData(gd);
      
		return dialogArea;
	}
	
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (textName.getText().equals(initialName) &&
		    textContent.getText().equals(initialContent))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newName = new String(textName.getText());
		final String newContent = new String(textContent.getText());
		new Job("Change policy") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					if (object != null)
					{
						NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
						md.setConfigFileName(newName);
						md.setConfigFileContent(newContent);
						NXMCSharedData.getInstance().getSession().modifyObject(md);
					}
					initialName = newName;
					initialContent = newContent;
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot change agent policy: " + e.getMessage(), e);
				}

				if (isApply)
				{
					new UIJob("Update \"Configuration File\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							ConfigFile.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}
}
