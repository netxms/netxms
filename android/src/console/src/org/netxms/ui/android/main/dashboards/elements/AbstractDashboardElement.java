/**
 * 
 */
package org.netxms.ui.android.main.dashboards.elements;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import org.netxms.ui.android.service.ClientConnectorService;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.util.Log;
import android.widget.FrameLayout;

/**
 * Base class for all dashboard elements
 */
public abstract class AbstractDashboardElement extends FrameLayout
{
	private static final String LOG_TAG = "nxclient/AbstractDashboardElement";
	
	protected static final int MAX_CHART_ITEMS = 16;
	protected static final int GRID_COLOR = 0xFFE8E8E8;
	protected static final int BACKGROUND_COLOR = 0xFFF0F0F0;
	protected static final int LABEL_COLOR = 0xFF000000;
	protected static final Integer[] DEFAULT_ITEM_COLORS = { 0x40699C, 0x9E413E, 0x7F9A48, 0x695185, 0x3C8DA3, 0xCC7B38, 0x4F81BD, 0xC0504D,
                                                            0x9BBB59, 0x8064A2, 0x4BACC6, 0xF79646, 0xAABAD7, 0xD9AAA9, 0xC6D6AC, 0xBAB0C9 };

	protected ClientConnectorService service;
	protected ScheduledExecutorService scheduleTaskExecutor;

	private Paint paint;
	private ScheduledFuture<?> task = null;
	
	/**
	 * @param context
	 * @param xmlConfig
	 */
	public AbstractDashboardElement(Context context, String xmlConfig, ClientConnectorService service, ScheduledExecutorService scheduleTaskExecutor)
	{
		super(context);
		this.service = service;
		this.scheduleTaskExecutor = scheduleTaskExecutor;
		setPadding(2, 2, 2, 2);
		setBackgroundColor(0xFFFFFFFF);
		paint = new Paint();
		paint.setStyle(Style.STROKE);
		paint.setColor(0xFFABADB3);
	}
	
	/* (non-Javadoc)
	 * @see android.view.View#onDraw(android.graphics.Canvas)
	 */
	@Override
	protected void onDraw(Canvas canvas)
	{
		super.onDraw(canvas);
		canvas.drawRect(0, 0, getWidth() - 1, getHeight() - 1, paint);
	}

	/**
	 * Refresh dashboard element
	 */
	public void refresh()
	{
	}
	
	/**
	 * Start element auto refresh at given interval
	 * 
	 * @param interval
	 */
	protected void startRefreshTask(int interval)
	{
		if (task != null)
		{
			Log.d(LOG_TAG, "startRefreshTask: timer already exist");
			return;
		}

		if (scheduleTaskExecutor == null)
		{
			Log.d(LOG_TAG, "startRefreshTask: executor service not available");
			return;
		}

		task = scheduleTaskExecutor.scheduleWithFixedDelay(new Runnable() {
			@Override
			public void run()
			{
				refresh();
			}
		}, 0, interval, TimeUnit.SECONDS);
		Log.d(LOG_TAG, "startRefreshTask: new task scheduled, interval=" + interval);
	}

	/* (non-Javadoc)
	 * @see android.view.View#onDetachedFromWindow()
	 */
	@Override
	protected void onDetachedFromWindow()
	{
		if (task != null)
		{
			Log.d(LOG_TAG, "onDetachedFromWindow: cancelling scheduled task");
			task.cancel(true);
			task = null;
		}
		super.onDetachedFromWindow();
	}

	/**
	 * Swap RGB color (R <--> B)
	 */
	protected int swapRGB(int color)
	{
		return ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);	// R | G | B
	}

	/**
	 * Swap RGB color (R <--> B) and set alpha to 255
	 */
	protected int toAndroidColor(int color)
	{
		return ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16) | 0xFF000000;	// R | G | B | A
	}
}
