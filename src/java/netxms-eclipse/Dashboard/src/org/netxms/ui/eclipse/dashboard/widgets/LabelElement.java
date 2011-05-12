/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.ui.eclipse.dashboard.widgets.internal.LabelConfig;
import org.netxms.ui.eclipse.tools.ColorConverter;

/**
 * @author victor
 *
 */
public class LabelElement extends ElementWidget
{
	private LabelConfig config; 
	private Label label;
	private Font font;
	
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
		
		FillLayout layout = new FillLayout();
		layout.marginHeight = 4;
		setLayout(layout);
		label = new Label(this, SWT.CENTER);
		label.setText(config.getTitle());
		label.setBackground(ColorConverter.colorFromInt(config.getBackgroundColorAsInt()));
		label.setForeground(ColorConverter.colorFromInt(config.getForegroundColorAsInt()));
		setBackground(ColorConverter.colorFromInt(config.getBackgroundColorAsInt()));
		
		font = new Font(getDisplay(), "Verdana", 10, SWT.BOLD);
		label.setFont(font);
		
		addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				font.dispose();
			}
		});
	}

}
