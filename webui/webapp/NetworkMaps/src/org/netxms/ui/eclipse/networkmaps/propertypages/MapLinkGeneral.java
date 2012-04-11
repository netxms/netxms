/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.ui.eclipse.networkmaps.views.helpers.LinkEditor;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "General" property page for map link
 */
public class MapLinkGeneral extends PropertyPage
{
	private LinkEditor object;
	private LabeledText name;
	private LabeledText connector1;
	private LabeledText connector2;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		object = (LinkEditor)getElement().getAdapter(LinkEditor.class);

		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		dialogArea.setLayout(layout);
		
		name = new LabeledText(dialogArea, SWT.NONE);
		name.setLabel("Name");
		name.setText(object.getName());
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		name.setLayoutData(gd);
		
		connector1 = new LabeledText(dialogArea, SWT.NONE);
		connector1.setLabel("Name for connector 1");
		connector1.setText(object.getConnectorName1());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		connector1.setLayoutData(gd);
		
		connector2 = new LabeledText(dialogArea, SWT.NONE);
		connector2.setLabel("Name for connector 2");
		connector2.setText(object.getConnectorName2());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		connector2.setLayoutData(gd);
		
		return dialogArea;
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	private boolean applyChanges(final boolean isApply)
	{
		object.setName(name.getText());
		object.setConnectorName1(connector1.getText());
		object.setConnectorName2(connector2.getText());
		object.update();
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		return applyChanges(false);
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
