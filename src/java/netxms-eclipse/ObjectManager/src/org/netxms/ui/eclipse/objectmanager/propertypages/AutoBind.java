/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.NXCContainer;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class AutoBind extends PropertyPage
{
	private NXCContainer object;
	private Button checkboxEnable;
	private Text filterSource;
	private boolean initialAutoBindFlag;
	private String initialAutoBindFilter;

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		object = (NXCContainer)getElement().getAdapter(NXCContainer.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		initialAutoBindFlag = object.isAutoBindEnabled();
		initialAutoBindFilter = new String(object.getAutoBindFilter());
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // Enable/disable check box
      checkboxEnable = new Button(dialogArea, SWT.CHECK);
      checkboxEnable.setText("Automatically bind nodes selected by filter to this container");
      checkboxEnable.setSelection(object.isAutoBindEnabled());
      checkboxEnable.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				if (checkboxEnable.getSelection())
				{
					filterSource.setEnabled(true);
					filterSource.setFocus();
				}
				else
				{
					filterSource.setEnabled(false);
				}
			}
      });
      
      // Filtering script
      Label label = new Label(dialogArea, SWT.NONE);
      label.setText("Filtering script");

      GridData gd = new GridData();
      gd.verticalIndent = IUIConstants.DIALOG_SPACING;
		label.setLayoutData(gd);
      
      filterSource = new Text(dialogArea, SWT.BORDER | SWT.MULTI);
		filterSource.setText(object.getAutoBindFilter());
		filterSource.setFont(new Font(getShell().getDisplay(), "Courier New", 10, SWT.NORMAL));
		filterSource.setEnabled(object.isAutoBindEnabled());
		
		gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		filterSource.setLayoutData(gd);
		
		return dialogArea;
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		final boolean isAutoBindEnabled = checkboxEnable.getSelection();
		if ((!isAutoBindEnabled && !initialAutoBindFlag) ||
		    (isAutoBindEnabled && initialAutoBindFlag && initialAutoBindFilter.equals(filterSource.getText())))
			return;		// Nothing to apply
		
		if (isApply)
			setValid(false);
		
		final String newAutoBindFilter = new String(filterSource.getText());
		new Job("Update auto-bind filter") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					if (object != null)
					{
						NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
						md.setAutoBindEnabled(isAutoBindEnabled);
						md.setAutoBindFilter(newAutoBindFilter);
						NXMCSharedData.getInstance().getSession().modifyObject(md);
					}
					initialAutoBindFlag = isAutoBindEnabled;
					initialAutoBindFilter = newAutoBindFilter;
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot change container automatic bind options: " + e.getMessage(), e);
				}

				if (isApply)
				{
					new UIJob("Update \"Automatic Bind Rules\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							AutoBind.this.setValid(true);
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
