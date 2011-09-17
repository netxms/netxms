/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

/**
 * @author victor
 *
 */
public class EditPaneWidget extends Canvas implements PaintListener
{
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 0, 0, 127);
	
	public EditPaneWidget(Composite parent)
	{
		super(parent, SWT.TRANSPARENT);
		addPaintListener(this);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		final GC gc = e.gc;
		final Point size = getSize();
		
		gc.setBackground(BACKGROUND_COLOR);
		gc.setAlpha(20);
		gc.fillRectangle(0, 0, size.x, size.y);
	}

}
