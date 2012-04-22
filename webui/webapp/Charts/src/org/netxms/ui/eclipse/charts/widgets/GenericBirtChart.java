/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.charts.widgets;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import org.eclipse.birt.chart.api.ChartEngine;
import org.eclipse.birt.chart.device.IDeviceRenderer;
import org.eclipse.birt.chart.exception.ChartException;
import org.eclipse.birt.chart.factory.GeneratedChartState;
import org.eclipse.birt.chart.factory.IGenerator;
import org.eclipse.birt.chart.model.Chart;
import org.eclipse.birt.chart.model.attribute.Bounds;
import org.eclipse.birt.chart.model.attribute.ColorDefinition;
import org.eclipse.birt.chart.model.attribute.Palette;
import org.eclipse.birt.chart.model.attribute.Position;
import org.eclipse.birt.chart.model.attribute.impl.BoundsImpl;
import org.eclipse.birt.chart.model.attribute.impl.ColorDefinitionImpl;
import org.eclipse.birt.chart.model.attribute.impl.PaletteImpl;
import org.eclipse.birt.core.framework.PlatformConfig;
import org.eclipse.jface.preference.PreferenceConverter;
import org.eclipse.rwt.graphics.Graphics;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ControlListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.ui.eclipse.charts.Activator;
import org.netxms.ui.eclipse.charts.api.ChartColor;
import org.netxms.ui.eclipse.tools.ColorCache;

/**
 * Abstract base class for all BIRT-based charts
 * 
 */
public abstract class GenericBirtChart extends GenericChart implements PaintListener, ControlListener
{
	private static final long serialVersionUID = 1L;
	
	private Chart chart = null;
	private boolean fullRepaint = true;
	private IDeviceRenderer deviceRenderer = null;
	private GeneratedChartState generatedChartState = null;
	private IGenerator generator = null;
	private Image imgChart = null;
	private ColorCache colors;
	private Set<String> errors = new HashSet<String>(0);
	private Image errorImage = null;

	/**
	 * Create chart widget
	 * 
	 * @param parent
	 *           parent widget
	 * @param style
	 *           widget style
	 */
	public GenericBirtChart(Composite parent, int style)
	{
		super(parent, style | SWT.NO_BACKGROUND);

		colors = new ColorCache(this);
		
		try
		{
			PlatformConfig config = new PlatformConfig();
			config.setBIRTHome("");
			final ChartEngine chartEngine = ChartEngine.instance(config);
			deviceRenderer = chartEngine.getRenderer("dv.PNG");
			generator = chartEngine.getGenerator();
		}
		catch(ChartException e)
		{
			/* TODO: implement logging or error reporting */
			e.printStackTrace();
		}

		addPaintListener(this);
		addControlListener(this);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#initializationComplete()
	 */
	@Override
	public void initializationComplete()
	{
		chart = createChart();
		fullRepaint = true;
		redraw();
	}

	/**
	 * Force chart recreation (for example, after chart type change).
	 */
	protected void recreateChart()
	{
		chart = createChart();
		fullRepaint = true;
		redraw();
	}

	/**
	 * Create underlying chart object. Must be implemented by subclasses.
	 * 
	 * @return BIRT chart object
	 */
	protected abstract Chart createChart();

	/**
	 * Get underlying BIRT chart object.
	 * 
	 * @return underlying BIRT chart object
	 */
	public Chart getChart()
	{
		return chart;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.events.ControlListener#controlMoved(org.eclipse.swt.events
	 * .ControlEvent)
	 */
	@Override
	public void controlMoved(ControlEvent arg0)
	{
		fullRepaint = true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.events.ControlListener#controlResized(org.eclipse.swt.
	 * events.ControlEvent)
	 */
	@Override
	public void controlResized(ControlEvent arg0)
	{
		fullRepaint = true;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.swt.events.PaintListener#paintControl(org.eclipse.swt.events
	 * .PaintEvent)
	 */
	@Override
	public final void paintControl(PaintEvent event)
	{
		if (chart == null)
			return;

		Rectangle clientArea = this.getClientArea();
		if (fullRepaint)
		{
			// if resized
			// imgChart is factory created and should not be disposed
			imgChart = null;

			try
			{
				File tmpFile = File.createTempFile("birt_" + hashCode(), "_" + clientArea.width + "_" + clientArea.height);

				deviceRenderer.setProperty(IDeviceRenderer.FILE_IDENTIFIER, tmpFile);
				final Bounds bounds = BoundsImpl.create(0, 0, clientArea.width, clientArea.height);
				bounds.scale(72d / deviceRenderer.getDisplayServer().getDpiResolution());

				generatedChartState = generator.build(deviceRenderer.getDisplayServer(), chart, bounds, null, null, null);
				generator.render(deviceRenderer, generatedChartState);

				InputStream inputStream = null;
				try
				{
					inputStream = new FileInputStream(tmpFile);
					imgChart = Graphics.getImage(tmpFile.getName(), inputStream);
					tmpFile.delete();
				}
				finally
				{
					if (inputStream != null)
					{
						inputStream.close();
					}
				}
			}
			catch(Exception e)
			{
				/* TODO: add logging and/or user notification */
				e.printStackTrace();
			}

			fullRepaint = false;
		}

		event.gc.drawImage(imgChart, clientArea.x, clientArea.y);
		paintErrorIndicator(event.gc);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#refresh()
	 */
	@Override
	public void refresh()
	{
		fullRepaint = true;
		redraw();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#rebuild()
	 */
	@Override
	public void rebuild()
	{
		recreateChart();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitle(java.lang.String)
	 */
	@Override
	public void setChartTitle(String title)
	{
		this.title = title;
		if (chart != null)
		{
			chart.getTitle().getLabel().getCaption().setValue(title);
			refresh();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setLegendVisible(boolean)
	 */
	@Override
	public void setLegendVisible(boolean visible)
	{
		legendVisible = visible;
		if (chart != null)
		{
			chart.getLegend().setVisible(visible);
			refresh();
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#setTitleVisible(boolean)
	 */
	@Override
	public void setTitleVisible(boolean visible)
	{
		titleVisible = visible;
		if (chart != null)
			chart.getTitle().setVisible(visible);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#set3DModeEnabled(boolean)
	 */
	@Override
	public void set3DModeEnabled(boolean enabled)
	{
		displayIn3D = enabled;
		if (chart != null)
			recreateChart();
	}

	/**
	 * Get palette usable by BIRT
	 * 
	 * @return BIRT palette object
	 */
	protected Palette getBirtPalette()
	{
		Palette birtPalette = PaletteImpl.create(0, true);
		for(ChartColor c : palette)
			birtPalette.getEntries().add(ColorDefinitionImpl.create(c.red, c.green, c.blue, c.alpha));
		return birtPalette;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.ui.eclipse.charts.api.DataChart#setLogScaleEnabled(boolean)
	 */
	@Override
	public void setLogScaleEnabled(boolean enabled)
	{
		if (useLogScale != enabled)
		{
			useLogScale = enabled;
			if (chart != null)
				recreateChart();
		}
	}

	/**
	 * Create BIRT color definition object from preference string
	 * 
	 * @param name
	 *           Preference name
	 * @return color definition object
	 */
	protected ColorDefinition getColorFromPreferences(final String name)
	{
		RGB rgb = PreferenceConverter.getColor(preferenceStore, name);
		return ColorDefinitionImpl.create(rgb.red, rgb.green, rgb.blue);
	}

	/**
	 * Get Position object from int constant
	 * 
	 * @param value
	 * @return
	 */
	protected Position positionFromInt(int value)
	{
		switch(value)
		{
			case GraphSettings.POSITION_LEFT:
				return Position.LEFT_LITERAL;
			case GraphSettings.POSITION_RIGHT:
				return Position.RIGHT_LITERAL;
			case GraphSettings.POSITION_TOP:
				return Position.ABOVE_LITERAL;
			case GraphSettings.POSITION_BOTTOM:
				return Position.BELOW_LITERAL;
		}
		return Position.RIGHT_LITERAL;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.ui.eclipse.charts.widgets.GenericChart#setLegendPosition(int)
	 */
	@Override
	public void setLegendPosition(int position)
	{
		super.setLegendPosition(position);
		if (chart != null)
			recreateChart();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.netxms.ui.eclipse.charts.widgets.GenericChart#setTranslucent(boolean)
	 */
	@Override
	public void setTranslucent(boolean translucent)
	{
		super.setTranslucent(translucent);
		if (chart != null)
			recreateChart();
	}
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#addError(java.lang.String)
	 */
	@Override
	public void addError(String message)
	{
		if (errors.add(message))
			redraw();
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.charts.api.DataChart#clearErrors()
	 */
	@Override
	public void clearErrors()
	{
		if (errors.size() > 0)
		{
			errors.clear();
			redraw();
		}
	}
	
	/**
	 * Draw error indicator if needed
	 * 
	 * @param gc
	 */
	private void paintErrorIndicator(GC gc)
	{
		if (errors.size() == 0)
			return;

		if (errorImage == null)
			errorImage = Activator.getImageDescriptor("icons/chart_error.png").createImage();

		gc.setAlpha(127);
		gc.setBackground(colors.create(127, 127, 127));
		gc.fillRectangle(getClientArea());
		gc.setAlpha(255);
		gc.drawImage(errorImage, 10, 10);
		
		gc.setForeground(colors.create(192, 0, 0));
		Iterator<String> it = errors.iterator();
		int y = 12;
		int h = gc.textExtent("X").y;
		while(it.hasNext())
		{
			gc.drawText(it.next(), 40, y, true);
			y += h + 5;
		}
	}
}
