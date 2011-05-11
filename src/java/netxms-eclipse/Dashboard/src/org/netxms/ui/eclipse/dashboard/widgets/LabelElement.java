/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;

/**
 * @author victor
 *
 */
public class LabelElement extends ElementWidget
{
	private LabelConfig config; 
	private Label label;
	
	/**
	 * @param parent
	 * @param data
	 */
	public LabelElement(Composite parent, String data)
	{
		super(parent, data);
		
		try
		{
			config = LabelConfig.createFromXml(data);
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new LabelConfig();
		}
		
		setLayout(new FillLayout());
		label = new Label(this, SWT.NONE);
		label.setText(config.getTitle());
	}

}
