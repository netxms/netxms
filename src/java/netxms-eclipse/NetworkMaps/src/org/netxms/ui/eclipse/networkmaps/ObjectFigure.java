/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps;

import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.MouseEvent;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.netxms.client.NXCObject;

/**
 * @author Victor
 *
 */
public class ObjectFigure extends Figure
{
	private static final Dimension corner = new Dimension(8, 8);

	private NXCObject object;
	private boolean selected = false;
	private Color bkColorNormal = new Color(null, 255, 255, 255);
	private Color bkColorSelected = new Color(null, 0, 0, 255);
	private Color borderColorNormal = new Color(null, 0, 0, 0);
	private Color borderColorSelected = new Color(null, 0, 0, 0);
	
	/**
	 * Create new object figure
	 * @param element
	 */
	public ObjectFigure(NXCObject object)
	{
		super();
		this.object = object;
		setSize(40, 40);
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics graphics)
	{
		super.paintFigure(graphics);
      
      Rectangle bounds = getBounds();

      graphics.setBackgroundColor(selected ? bkColorSelected : bkColorNormal);
      graphics.setAntialias(SWT.OFF);
      graphics.fillRoundRectangle(bounds, corner.width, corner.height);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintBorder(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintBorder(Graphics graphics)
	{
		super.paintBorder(graphics);

		graphics.setBackgroundColor(selected ? borderColorSelected : borderColorNormal);
      graphics.setAntialias(SWT.ON);
      graphics.drawRoundRectangle(bounds.getResized(-1, -1), corner.width, corner.height);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#handleMouseHover(org.eclipse.draw2d.MouseEvent)
	 */
	@Override
	public void handleMouseHover(MouseEvent event)
	{
		MessageDialog.openInformation(null, "mouse", "hover");
	}
}
