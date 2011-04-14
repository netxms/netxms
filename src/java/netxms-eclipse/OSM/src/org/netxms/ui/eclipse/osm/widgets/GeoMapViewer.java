/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.osm.widgets;

import java.net.MalformedURLException;
import java.net.URL;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.ui.progress.IWorkbenchSiteProgressService;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.GeoLocation;
import org.netxms.ui.eclipse.osm.tools.MapAccessor;

/**
 * This widget shows map retrieved via OpenStreetMap Static Map API
 */
public class GeoMapViewer extends Canvas implements PaintListener
{
	private static final Color INFO_BLOCK_BACKGROUND = new Color(Display.getDefault(), 255, 242, 0);
	private static final Color INFO_BLOCK_BORDER = new Color(Display.getDefault(), 0, 0, 0);
	private static final Color INFO_BLOCK_TEXT = new Color(Display.getDefault(), 0, 0, 0);
	
	private String imageAccessSync = "SYNC";
	private Image currentImage = null;
	private MapAccessor accessor;
	private IWorkbenchSiteProgressService siteService = null;
	private Image centerMarker = null;
	
	/**
	 * @param parent
	 * @param style
	 */
	public GeoMapViewer(Composite parent, int style)
	{
		super(parent, style);
		setLayout(new FillLayout());

		addPaintListener(this);
		
		final Runnable timer = new Runnable() {
			@Override
			public void run()
			{
				if (isDisposed())
					return;
				reloadMap();
			}
		};
		
		addListener(SWT.Resize, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				getDisplay().timerExec(1000, timer);
			}
		});
	}
	
	/**
	 * Show given map
	 * 
	 * @param accessor
	 */
	public void showMap(MapAccessor accessor)
	{
		this.accessor = new MapAccessor(accessor);
		reloadMap();
	}
	
	/**
	 * Reload current map
	 */
	private void reloadMap()
	{
		Rectangle rect = this.getClientArea();
		accessor.setMapWidth(rect.width);
		accessor.setMapHeight(rect.height);
		
		synchronized(imageAccessSync)
		{
			if (currentImage != null)
				currentImage.dispose();
			currentImage = null;
			redraw();
		}
		
		if (!accessor.isValid())
			return;

		URL url = null;
		try
		{
			url = new URL(accessor.generateUrl());
		}
		catch(MalformedURLException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
			return;
		}
		
		System.out.println("URL: " + url.toString());
		
		final ImageDescriptor id = ImageDescriptor.createFromURL(url);
		Job job = new Job("Download map image") {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				Image image = id.createImage();
				synchronized(imageAccessSync)
				{
					if (currentImage != null)
						currentImage.dispose();
					currentImage = image;
				}
				new UIJob("Redraw map") {
					@Override
					public IStatus runInUIThread(IProgressMonitor monitor)
					{
						GeoMapViewer.this.redraw();
						return Status.OK_STATUS;
					}
				}.schedule();
				return Status.OK_STATUS;
			}
		};
		if (siteService != null)
			siteService.schedule(job, 0, true);
		else
			job.schedule();
	}

	/**
	 * @return the siteService
	 */
	public IWorkbenchSiteProgressService getSiteService()
	{
		return siteService;
	}

	/**
	 * @param siteService the siteService to set
	 */
	public void setSiteService(IWorkbenchSiteProgressService siteService)
	{
		this.siteService = siteService;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events.PaintEvent)
	 */
	@Override
	public void paintControl(PaintEvent e)
	{
		final GC gc = e.gc;
		
		int imgW, imgH;
		synchronized(imageAccessSync)
		{
			if (currentImage != null)
			{
				gc.drawImage(currentImage, 0, 0);
				imgW = currentImage.getImageData().width;
				imgH = currentImage.getImageData().height;
			}
			else
			{
				gc.drawString("Loading...", 5, 5);
				imgW = -1;
				imgH = -1;
			}
		}
		
		if ((centerMarker != null) && (imgW > 0) && (imgH > 0))
		{
			int w = centerMarker.getImageData().width;
			int h = centerMarker.getImageData().height;
			gc.drawImage(centerMarker, (imgW - w) / 2, (imgH - h) / 2);
		}
		
		final GeoLocation gl = new GeoLocation(accessor.getLatitude(), accessor.getLongitude());
		final String text = gl.toString();
		final Point textSize = gc.textExtent(text); 
		
		Rectangle rect = getClientArea();
		rect.x = 10;
		//rect.x = rect.width - textSize.x - 20;
		rect.y += 10;
		rect.width = textSize.x + 10;
		rect.height = textSize.y + 8;
		
		gc.setAntialias(SWT.ON);
		gc.setBackground(INFO_BLOCK_BACKGROUND);
		gc.setAlpha(127);
		gc.fillRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		gc.setAlpha(255);
		gc.setForeground(INFO_BLOCK_BORDER);
		gc.setLineWidth(1);
		gc.drawRoundRectangle(rect.x, rect.y, rect.width, rect.height, 8, 8);
		
		gc.setForeground(INFO_BLOCK_TEXT);
		gc.drawText(text, rect.x + 5, rect.y + 4, true);
	}

	/**
	 * @return the centerMarker
	 */
	public Image getCenterMarker()
	{
		return centerMarker;
	}

	/**
	 * @param centerMarker the centerMarker to set
	 */
	public void setCenterMarker(Image centerMarker)
	{
		this.centerMarker = centerMarker;
	}
}
