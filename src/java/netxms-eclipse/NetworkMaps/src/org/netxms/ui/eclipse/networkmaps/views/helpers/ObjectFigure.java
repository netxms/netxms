/**
 * 
 */
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.BorderLayout;
import org.eclipse.draw2d.Figure;
import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.StatusDisplayInfo;

/**
 * @author victor
 *
 */
public class ObjectFigure extends Figure
{
	private static final int IMAGE_SIZE_X = 32;
	private static final int IMAGE_SIZE_Y = 32;
	private static final int IMAGE_MARGIN_X = 12;
	private static final int IMAGE_MARGIN_Y = 4;
	private static final int BACKGROUND_MARGIN_X = 4;
	private static final int BACKGROUND_MARGIN_Y = 4;
	
	private GenericObject object;
	private MapLabelProvider labelProvider;
	private Label label;
	
	/**
	 * Constructor
	 * @param object Object represented by this figure
	 */
	public ObjectFigure(GenericObject object, MapLabelProvider labelProvider)
	{
		this.object = object;
		this.labelProvider = labelProvider;
		
		setLayoutManager(new BorderLayout());
		
		label = new Label(object.getObjectName());
		label.setFont(labelProvider.getFont());
		label.setLabelAlignment(PositionConstants.CENTER);
		add(label, BorderLayout.BOTTOM);
		
		Dimension ls = label.getPreferredSize(IMAGE_SIZE_X + IMAGE_MARGIN_X * 2, -1);
		setSize(IMAGE_SIZE_X + IMAGE_MARGIN_X * 2, IMAGE_SIZE_Y + IMAGE_MARGIN_Y * 2 + ls.height);
		
		setToolTip(new ObjectTooltip(object));
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		Rectangle rect = new Rectangle(getBounds());
		
		// Status background
		if (labelProvider.isShowStatusBackground())
		{
			rect.x += IMAGE_MARGIN_X - BACKGROUND_MARGIN_X;
			rect.y += IMAGE_MARGIN_Y - BACKGROUND_MARGIN_Y;
			rect.width = IMAGE_SIZE_X + BACKGROUND_MARGIN_X * 2;
			rect.height = IMAGE_SIZE_Y + BACKGROUND_MARGIN_Y * 2;
			
			gc.setBackgroundColor(StatusDisplayInfo.getStatusColor(object.getStatus()));
			gc.setAntialias(SWT.ON);
			gc.setAlpha(64);
			gc.fillRoundRectangle(rect, 16, 16);
			gc.setAlpha(255);

			rect = new Rectangle(getBounds());
		}
			
		// Object image
		Image image = labelProvider.getImage(object);
		if (image != null)
		{
			gc.drawImage(image, rect.x + IMAGE_MARGIN_X, rect.y + IMAGE_MARGIN_Y);
		}
		
		// Status image
		if (labelProvider.isShowStatusIcons())
		{
			image = labelProvider.getStatusImage(object);
			if (image != null)
			{
				org.eclipse.swt.graphics.Rectangle imgSize = image.getBounds();
				gc.drawImage(image, rect.x + rect.width - imgSize.width, rect.y);  // rect.y + rect.height - imgSize.height
			}
		}
	}
}
