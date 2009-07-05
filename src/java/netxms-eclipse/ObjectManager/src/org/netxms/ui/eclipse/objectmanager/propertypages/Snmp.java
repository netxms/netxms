/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCNode;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class Snmp extends PropertyPage
{
	private NXCNode node;
	private Combo snmpVersion;
	
	/**
	 * 
	 */
	public Snmp()
	{
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		node = (NXCNode)getElement().getAdapter(NXCNode.class);
		if (node == null)	// Paranoid check
			return dialogArea;
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
      dialogArea.setLayout(layout);

      // SNMP version selection
      Group version = new Group(dialogArea, SWT.NONE);
      version.setText("SNMP Version");
      GridLayout versionLayout = new GridLayout();
      versionLayout.numColumns = 3;
      version.setLayout(versionLayout);
      
      Button version1 = new Button(version, SWT.RADIO);
      version1.setText("1");

      Button version2c = new Button(version, SWT.RADIO);
      version2c.setText("2c");
		
      Button version3 = new Button(version, SWT.RADIO);
      version3.setText("3");
      
		return dialogArea;
	}
}
