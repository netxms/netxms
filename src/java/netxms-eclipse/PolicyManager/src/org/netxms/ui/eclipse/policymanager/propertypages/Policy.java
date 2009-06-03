/**
 * 
 */
package org.netxms.ui.eclipse.policymanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCAgentPolicy;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.ui.eclipse.policymanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class Policy extends PropertyPage
{
	private Text textDescription;
	private String initialDescription;
	private NXCAgentPolicy object;
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NXCAgentPolicy)getElement().getAdapter(NXCAgentPolicy.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);
      
		// Object name
      initialDescription = new String(object.getDescription());
      textDescription = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, "Description",
      		                                    initialDescription, null);
		
		return dialogArea;
	}
	
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (textDescription.getText().equals(initialDescription))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newDescription = new String(textDescription.getText());
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
						md.setDescription(newDescription);
						NXMCSharedData.getInstance().getSession().modifyObject(md);
					}
					initialDescription = newDescription;
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
					new UIJob("Update \"Policy\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							Policy.this.setValid(true);
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
