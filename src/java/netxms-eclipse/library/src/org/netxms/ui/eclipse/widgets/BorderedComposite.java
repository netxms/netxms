/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

/**
 * Composite with lightweight border (Windows 7 style)
 *
 */
public class BorderedComposite extends Composite implements PaintListener
{
	private static final Color BORDER_OUTER_COLOR = new Color(Display.getDefault(), 171, 173, 179);
	private static final Color BORDER_INNER_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	
	/**
	 * @param parent
	 * @param style
	 */
	public BorderedComposite(Composite parent, int style)
	{
		super(parent, style & ~SWT.BORDER);
		addPaintListener(this);
		setBackground(BACKGROUND_COLOR);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Scrollable#computeTrim(int, int, int, int)
	 */
	@Override
	public Rectangle computeTrim(int x, int y, int width, int height)
	{
		Rectangle trim = super.computeTrim(x, y, width, height);
		trim.x -= 2;
		trim.y -= 2;
		trim.width += 4;
		trim.height += 4;
		return trim;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Scrollable#getClientArea()
	 */
	@Override
	public Rectangle getClientArea()
	{
		Rectangle area = super.getClientArea();
		area.x += 2;
		area.y += 2;
		area.width -= 4;
		area.height -= 4;
		return area;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Control#getBorderWidth()
	 */
	@Override
	public int getBorderWidth()
	{
		return 2;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		Point size = getSize();
		Rectangle rect = new Rectangle(0, 0, size.x, size.y);
		
		rect.width--;
		rect.height--;
		e.gc.setForeground(BORDER_OUTER_COLOR);
		e.gc.drawRectangle(rect);
		
		rect.x++;
		rect.y++;
		rect.width -= 2;
		rect.height -= 2;
		e.gc.setForeground(BORDER_INNER_COLOR);
		e.gc.drawRectangle(rect);
	}
}
