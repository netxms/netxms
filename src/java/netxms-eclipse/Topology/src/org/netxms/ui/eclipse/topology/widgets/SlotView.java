/**
 * 
 */
package org.netxms.ui.eclipse.topology.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;

/**
 * Single slot view
 *
 */
public class SlotView extends Canvas implements PaintListener
{
	/**
	 * @param parent
	 * @param style
	 */
	public SlotView(Composite parent, int style)
	{
		super(parent, style | SWT.BORDER);
		addPaintListener(this);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		// TODO Auto-generated method stub
		
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		return new Point(100, 100);
	}

}
