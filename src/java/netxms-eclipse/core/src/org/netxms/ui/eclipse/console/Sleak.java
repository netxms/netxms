package org.netxms.ui.eclipse.console;

/*
 * Copyright (c) 2000, 2002 IBM Corp.  All rights reserved.
 * This file is made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/cpl-v10.html
 */
import java.io.ByteArrayOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.DeviceData;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.graphics.Region;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.List;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.MessageBox;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;

public class Sleak
{
	private Display display;
	private Shell shell;
	private List list;
	private Canvas canvas;
	private Button start, stop, check;
	private Text text;

	private Label label;
	private Object[] oldObjects = new Object[0];
	private Object[] objects = new Object[0];

	private Error[] errors = new Error[0];

	private String traceFileName;

	public Sleak()
	{
		traceFileName = System.getProperty("sleak");
		if (traceFileName.trim().isEmpty())
		{
			traceFileName = null;
		}
	}

	void layout()
	{
		final Rectangle rect = shell.getClientArea();
		int width = 0;
		final String[] items = list.getItems();
		final GC gc = new GC(list);
		for(int i = 0; i < objects.length; i++)
		{
			width = Math.max(width, gc.stringExtent(items[i]).x);
		}
		gc.dispose();
		final Point size1 = start.computeSize(SWT.DEFAULT, SWT.DEFAULT);
		final Point size2 = stop.computeSize(SWT.DEFAULT, SWT.DEFAULT);
		final Point size3 = check.computeSize(SWT.DEFAULT, SWT.DEFAULT);
		final Point size4 = label.computeSize(SWT.DEFAULT, SWT.DEFAULT);
		width = Math.max(size1.x, Math.max(size2.x, Math.max(size3.x, width)));
		width = Math.max(64, Math.max(size4.x, list.computeSize(width, SWT.DEFAULT).x));
		start.setBounds(0, 0, width, size1.y);
		stop.setBounds(0, size1.y, width, size2.y);
		check.setBounds(0, size1.y + size2.y, width, size3.y);
		label.setBounds(0, rect.height - size4.y, width, size4.y);
		final int height = size1.y + size2.y + size3.y;
		list.setBounds(0, height, width, rect.height - height - size4.y);
		text.setBounds(width, 0, rect.width - width, rect.height);
		canvas.setBounds(width, 0, rect.width - width, rect.height);
	}

	String objectName(final Object object)
	{
		final String string = object.toString();
		final int index = string.lastIndexOf('.');
		if (index == -1)
		{
			return string;
		}
		return string.substring(index + 1, string.length());
	}

	public void open()
	{
		display = Display.getCurrent();
		shell = new Shell(display);
		shell.setText("S-Leak");
		list = new List(shell, SWT.BORDER | SWT.V_SCROLL);
		list.addListener(SWT.Selection, new Listener()
		{
			@Override
			public void handleEvent(final Event event)
			{
				refreshObject();
			}
		});
		text = new Text(shell, SWT.BORDER | SWT.H_SCROLL | SWT.V_SCROLL);
		canvas = new Canvas(shell, SWT.BORDER);
		canvas.addListener(SWT.Paint, new Listener()
		{
			@Override
			public void handleEvent(final Event event)
			{
				paintCanvas(event);
			}
		});
		check = new Button(shell, SWT.CHECK);
		check.setText("Stack");
		check.addListener(SWT.Selection, new Listener()
		{
			@Override
			public void handleEvent(final Event e)
			{
				toggleStackTrace();
			}
		});
		start = new Button(shell, SWT.PUSH);
		start.setText("Snap");
		start.addListener(SWT.Selection, new Listener()
		{
			@Override
			public void handleEvent(final Event event)
			{
				refreshAll();
			}
		});
		stop = new Button(shell, SWT.PUSH);
		stop.setText("Diff");
		stop.addListener(SWT.Selection, new Listener()
		{
			@Override
			public void handleEvent(final Event event)
			{
				refreshDifference();
			}
		});
		label = new Label(shell, SWT.BORDER);
		label.setText("0 object(s)");
		shell.addListener(SWT.Resize, new Listener()
		{
			@Override
			public void handleEvent(final Event e)
			{
				layout();
			}
		});
		check.setSelection(false);
		text.setVisible(false);
		final Point size = shell.getSize();
		shell.setSize(size.x / 2, size.y / 2);
		shell.open();
	}

	void paintCanvas(final Event event)
	{
		canvas.setCursor(null);
		final int index = list.getSelectionIndex();
		if (index == -1)
		{
			return;
		}
		final GC gc = event.gc;
		final Object object = objects[index];
		if (object instanceof Color)
		{
			if (((Color)object).isDisposed())
			{
				return;
			}
			gc.setBackground((Color)object);
			gc.fillRectangle(canvas.getClientArea());
			return;
		}
		if (object instanceof Cursor)
		{
			if (((Cursor)object).isDisposed())
			{
				return;
			}
			canvas.setCursor((Cursor)object);
			return;
		}
		if (object instanceof Font)
		{
			if (((Font)object).isDisposed())
			{
				return;
			}
			gc.setFont((Font)object);
			final FontData[] array = gc.getFont().getFontData();
			String string = "";
			final String lf = text.getLineDelimiter();
			for(final FontData data : array)
			{
				String style = "NORMAL";
				final int bits = data.getStyle();
				if (bits != 0)
				{
					if ((bits & SWT.BOLD) != 0)
					{
						style = "BOLD ";
					}
					if ((bits & SWT.ITALIC) != 0)
					{
						style += "ITALIC";
					}
				}
				string += data.getName() + " " + data.getHeight() + " " + style + lf;
			}
			gc.drawString(string, 0, 0);
			return;
		}
		// NOTHING TO DRAW FOR GC
		// if (object instanceof GC) {
		// return;
		// }
		if (object instanceof Image)
		{
			if (((Image)object).isDisposed())
			{
				return;
			}
			gc.drawImage((Image)object, 0, 0);
			return;
		}
		if (object instanceof Region)
		{
			if (((Region)object).isDisposed())
			{
				return;
			}
			final String string = ((Region)object).getBounds().toString();
			gc.drawString(string, 0, 0);
			return;
		}
	}

	void refreshAll()
	{
		oldObjects = new Object[0];
		refreshDifference();
		oldObjects = objects;
	}

	void refreshDifference()
	{
		final DeviceData info = display.getDeviceData();
		if (!info.tracking)
		{
			final MessageBox dialog = new MessageBox(shell, SWT.ICON_WARNING | SWT.OK);
			dialog.setText(shell.getText());
			dialog.setMessage("Warning: Device is not tracking resource allocation");
			dialog.open();
		}
		final Object[] newObjects = info.objects;
		final Error[] newErrors = info.errors;
		final Object[] diffObjects = new Object[newObjects.length];
		final Error[] diffErrors = new Error[newErrors.length];
		int count = 0;
		for(int i = 0; i < newObjects.length; i++)
		{
			int index = 0;
			while(index < oldObjects.length)
			{
				if (newObjects[i] == oldObjects[index])
				{
					break;
				}
				index++;
			}
			if (index == oldObjects.length)
			{
				diffObjects[count] = newObjects[i];
				diffErrors[count] = newErrors[i];
				count++;
			}
		}
		objects = new Object[count];
		errors = new Error[count];
		System.arraycopy(diffObjects, 0, objects, 0, count);
		System.arraycopy(diffErrors, 0, errors, 0, count);
		list.removeAll();
		text.setText("");
		canvas.redraw();

		if (traceFileName != null)
		{
			writeTraceFile(count);
		}

		for(final Object object : objects)
		{
			list.add(objectName(object));
		}
		refreshLabel();
		layout();
	}

	void refreshLabel()
	{
		int colors = 0, cursors = 0, fonts = 0, gcs = 0, images = 0;
		for(final Object object : objects)
		{
			if (object instanceof Color)
			{
				colors++;
			}
			if (object instanceof Cursor)
			{
				cursors++;
			}
			if (object instanceof Font)
			{
				fonts++;
			}
			if (object instanceof GC)
			{
				gcs++;
			}
			if (object instanceof Image)
			{
				images++;
			}
			if (object instanceof Region)
			{
			}
		}
		String string = "";
		if (colors != 0)
		{
			string += colors + " Color(s)\n";
		}
		if (cursors != 0)
		{
			string += cursors + " Cursor(s)\n";
		}
		if (fonts != 0)
		{
			string += fonts + " Font(s)\n";
		}
		if (gcs != 0)
		{
			string += gcs + " GC(s)\n";
		}
		if (images != 0)
		{
			string += images + " Image(s)\n";
		}
		/* Currently regions are not counted. */
		// if (regions != 0) string += regions + " Region(s)\n";
		if (string.length() != 0)
		{
			string = string.substring(0, string.length() - 1);
		}
		label.setText(string);
	}

	void refreshObject()
	{
		final int index = list.getSelectionIndex();
		if (index == -1)
		{
			return;
		}
		if (check.getSelection())
		{
			final ByteArrayOutputStream stream = new ByteArrayOutputStream();
			final PrintStream s = new PrintStream(stream);
			errors[index].printStackTrace(s);
			text.setText(stream.toString());
			text.setVisible(true);
			canvas.setVisible(false);
		}
		else
		{
			canvas.setVisible(true);
			text.setVisible(false);
			canvas.redraw();
		}
	}

	void toggleStackTrace()
	{
		refreshObject();
		layout();
	}

	private void writeTraceFile(final int count)
	{
		PrintWriter writer = null;
		try
		{
			writer = new PrintWriter(new FileWriter(traceFileName));
		}
		catch(final IOException e)
		{
			// e.printStackTrace();
		}

		for(int i = 0; i < count; i++)
		{
			if (writer != null)
			{
				writer.println(objects[i].toString());
				writer.println();
				final ByteArrayOutputStream stream = new ByteArrayOutputStream();
				final PrintStream s = new PrintStream(stream);
				errors[i].printStackTrace(s);
				writer.println(stream.toString());
				writer.println("--------------------------");
			}
		}

		if (writer != null)
		{
			writer.close();
		}
	}

}
