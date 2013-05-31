/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.core.runtime.IProduct;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.resource.StringConverter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.branding.IProductConstants;
import org.eclipse.ui.splash.BasicSplashHandler;
import org.netxms.base.NXCommon;

/**
 * Custom splash handler
 */
public class SplashHandler extends BasicSplashHandler
{
	private static SplashHandler instance;
	
	/**
	 * @return the instance
	 */
	public static SplashHandler getInstance()
	{
		return instance;
	}
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.splash.AbstractSplashHandler#init(org.eclipse.swt.widgets.Shell)
	 */
	public void init(Shell splash)
	{
		super.init(splash);
		instance = this;
		String progressRectString = null;
		String messageRectString = null;
		String foregroundColorString = null;
		IProduct product = Platform.getProduct();
		if (product != null)
		{
			progressRectString = product.getProperty(IProductConstants.STARTUP_PROGRESS_RECT);
			messageRectString = product.getProperty(IProductConstants.STARTUP_MESSAGE_RECT);
			foregroundColorString = product.getProperty(IProductConstants.STARTUP_FOREGROUND_COLOR);
		}
		Rectangle progressRect = StringConverter.asRectangle(progressRectString, new Rectangle(10, 10, 300, 15));
		setProgressRect(progressRect);

		Rectangle messageRect = StringConverter.asRectangle(messageRectString, new Rectangle(10, 35, 300, 15));
		setMessageRect(messageRect);

		int foregroundColorInteger;
		try
		{
			foregroundColorInteger = Integer.parseInt(foregroundColorString, 16);
		}
		catch(Exception ex)
		{
			foregroundColorInteger = 0xD2D7FF; // off white
		}
		
		setForeground(new RGB((foregroundColorInteger & 0xFF0000) >> 16, (foregroundColorInteger & 0xFF00) >> 8, foregroundColorInteger & 0xFF));
		
		final Color versionColor = new Color(Display.getCurrent(), 56, 56, 52);
		final Font versionFont = new Font(Display.getCurrent(), "Verdana", 9, SWT.BOLD); //$NON-NLS-1$
		Composite content = getContent();
		content.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				e.gc.setForeground(versionColor);
				e.gc.setFont(versionFont);
				e.gc.drawText(Messages.SplashHandler_Version + NXCommon.VERSION, 209, 181, true);
			}
		});
		content.addDisposeListener(new DisposeListener() {
			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				versionColor.dispose();
				versionFont.dispose();
			}
		});
	}
}
