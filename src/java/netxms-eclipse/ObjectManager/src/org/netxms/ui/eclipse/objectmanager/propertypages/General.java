/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObject;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class General extends PropertyPage
{
	private Text textName;
	private String initialName;
	
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		NXCObject object = (NXCObject)getElement().getAdapter(NXCObject.class);
		if (object == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
//      layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
//      layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
      
      // Object ID
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, "Object ID",
                                     Long.toString(object.getObjectId()), null);
      
		// Object class
      WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY, "Object class",
                                     Long.toString(object.getObjectClass()), null);
		
		// Object name
      initialName = new String(object.getObjectName());
      textName = WidgetHelper.createLabeledText(dialogArea, SWT.SINGLE | SWT.BORDER, "Object name",
      		                                    initialName, null);
		
		return dialogArea;
	}

	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		if (!textName.getText().equals(initialName))
		{
			MessageDialog.openInformation(null, "OK", textName.getText());
		}
		return true;
	}
}
