/**
 * 
 */
package org.netxms.ui.eclipse.widgets;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.ImageLoader;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;

/**
 * Displays animated image
 */
public class AnimatedImage extends Canvas implements PaintListener, DisposeListener
{
	private GC memGC = null;
	private Image image = null;
	private ImageData[] imageData = null;
	private int imageNumber;
	private Runnable timer;
	
	/**
	 * @param parent
	 * @param style
	 */
	public AnimatedImage(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);
		addPaintListener(this);
		addDisposeListener(this);
		
		timer = new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed() || (imageData == null))
					return;
				
				imageNumber++;
				if (imageNumber == imageData.length)
					imageNumber = 0;
				
				final Image frameImage = new Image(getDisplay(), imageData[imageNumber]);
				//memGC.fillRectangle(imageData[imageNumber].x, imageData[imageNumber].y, imageData[imageNumber].width, imageData[imageNumber].height);
	         memGC.drawImage(frameImage, imageData[imageNumber].x, imageData[imageNumber].y);
	         frameImage.dispose();
	         redraw();
				
				scheduleAnimation();
			}
		};
	}

	/**
	 * Load animated image from URL. For loading image from
	 * plugin, use URL like platform:/plugin/<plugin_id>/<path>
	 * 
	 * @param url image file's URL
	 */
	public void setImage(URL url)
	{
		getDisplay().timerExec(-1, timer);	// cancel animation timer
		
		if (memGC != null)
		{
			memGC.dispose();
			memGC = null;
		}
		
		if (image != null)
		{
			image.dispose();
			image = null;
		}
		
		if (url == null)
		{
			imageData = null;
			return;
		}
		
		ImageLoader loader = new ImageLoader();
		InputStream inputStream = null;
		try
		{
			inputStream = url.openConnection().getInputStream();
			imageData = loader.load(inputStream);
			imageNumber = 0;
			
			image = new Image(getDisplay(), imageData[0]);
			memGC = new GC(image);
			
			setSize(imageData[0].width, imageData[0].height);
			redraw();
			if (imageData.length > 1)
				scheduleAnimation();
		}
		catch(Exception e)
		{
			imageData = null;
		}
		finally
		{
			if (inputStream != null)
			{
				try
				{
					inputStream.close();
				}
				catch(IOException e)
				{
				}
			}
		}
	}

	/**
	 * Schedule next animation step
	 */
	private void scheduleAnimation()
	{
		if (imageData == null)
			return;
		
		getDisplay().timerExec((imageData[imageNumber].delayTime > 0) ? imageData[imageNumber].delayTime * 10 : 200, timer);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		if (image != null)
			e.gc.drawImage(image, 0, 0);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.DisposeListener#widgetDisposed(org.eclipse.swt.events.DisposeEvent)
	 */
	@Override
	public void widgetDisposed(DisposeEvent e)
	{
		if (memGC != null)
			memGC.dispose();
		if (image != null)
			image.dispose();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
	 */
	@Override
	public Point computeSize(int wHint, int hHint, boolean changed)
	{
		if ((imageData == null) || (imageData.length == 0))
			return super.computeSize(wHint, hHint, changed);
		int bw = getBorderWidth() * 2;
		return new Point(imageData[0].width + bw, imageData[0].height + bw);
	}
}
