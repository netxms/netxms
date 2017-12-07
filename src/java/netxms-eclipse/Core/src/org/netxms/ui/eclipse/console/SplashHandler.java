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
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.branding.IProductConstants;
import org.eclipse.ui.splash.BasicSplashHandler;
import org.netxms.base.BuildNumber;
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
		if (progressRectString != null)
		{
			Rectangle progressRect = StringConverter.asRectangle(progressRectString, new Rectangle(10, 10, 300, 15));
			setProgressRect(progressRect);
		}

		if (messageRectString != null)
		{
			Rectangle messageRect = StringConverter.asRectangle(messageRectString, new Rectangle(10, 35, 300, 15));
			setMessageRect(messageRect);
		}

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
		
		final Color versionColor = new Color(Display.getCurrent(), 224, 224, 224);
		final Font versionFont = new Font(Display.getCurrent(), "Verdana", 9, SWT.NONE); //$NON-NLS-1$
		final String versionText = Messages.get().SplashHandler_Version + NXCommon.VERSION + " (" + BuildNumber.TEXT + ")"; //$NON-NLS-1$ //$NON-NLS-2$
		final String copyrightText = "\u00A9 2003-2017 Raden Solutions"; //$NON-NLS-1$
      final Composite content = getContent();
		content.addPaintListener(new PaintListener() {
			@Override
			public void paintControl(PaintEvent e)
			{
				e.gc.setForeground(versionColor);
				e.gc.setFont(versionFont);
				int y = 240;
            Point size = e.gc.textExtent(copyrightText);
            e.gc.drawText(copyrightText, 485 - size.x, y, true);
            y += size.y + 2;
				size = e.gc.textExtent(versionText);
				e.gc.drawText(versionText, 485 - size.x, y, true);
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
