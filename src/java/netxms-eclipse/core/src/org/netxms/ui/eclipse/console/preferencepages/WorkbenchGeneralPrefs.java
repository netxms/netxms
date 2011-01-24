/**
 * 
 */
package org.netxms.ui.eclipse.console.preferencepages;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPreferencePage;
import org.eclipse.ui.PlatformUI;
import org.netxms.ui.eclipse.console.Activator;
import org.netxms.ui.eclipse.console.Messages;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author victor
 *
 */
public class WorkbenchGeneralPrefs extends PreferencePage implements	IWorkbenchPreferencePage
{
	private Button cbShowHeapMonitor;
	private Button cbShowTrayIcon;
	private Button cbHideWhenMinimized;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		dialogArea.setLayout(layout);
		
		cbShowHeapMonitor = new Button(dialogArea, SWT.CHECK);
		cbShowHeapMonitor.setText(Messages.getString("WorkbenchGeneralPrefs.show_heap")); //$NON-NLS-1$
		cbShowHeapMonitor.setSelection(getPreferenceStore().getBoolean("SHOW_MEMORY_MONITOR")); //$NON-NLS-1$
		
		cbShowTrayIcon = new Button(dialogArea, SWT.CHECK);
		cbShowTrayIcon.setText(Messages.getString("WorkbenchGeneralPrefs.show_tray_icon")); //$NON-NLS-1$
		cbShowTrayIcon.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("SHOW_TRAY_ICON")); //$NON-NLS-1$
		
		cbHideWhenMinimized = new Button(dialogArea, SWT.CHECK);
		cbHideWhenMinimized.setText("Hide when minimized");
		cbHideWhenMinimized.setSelection(Activator.getDefault().getPreferenceStore().getBoolean("HIDE_WHEN_MINIMIZED")); //$NON-NLS-1$
		
		return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IWorkbenchPreferencePage#init(org.eclipse.ui.IWorkbench)
	 */
	@Override
	public void init(IWorkbench workbench)
	{
		setPreferenceStore(PlatformUI.getPreferenceStore());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		cbShowHeapMonitor.setSelection(true);
		cbShowTrayIcon.setSelection(true);
		cbHideWhenMinimized.setSelection(false);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		getPreferenceStore().setValue("SHOW_MEMORY_MONITOR", cbShowHeapMonitor.getSelection()); //$NON-NLS-1$
		Activator.getDefault().getPreferenceStore().setValue("SHOW_TRAY_ICON", cbShowTrayIcon.getSelection()); //$NON-NLS-1$
		Activator.getDefault().getPreferenceStore().setValue("HIDE_WHEN_MINIMIZED", cbHideWhenMinimized.getSelection()); //$NON-NLS-1$
		
		if (cbShowTrayIcon.getSelection())
			Activator.showTrayIcon();
		else
			Activator.hideTrayIcon();
		
		return super.performOk();
	}
}
