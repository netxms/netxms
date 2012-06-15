package org.netxms.ui.android.main.views;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import android.content.Context;
import android.graphics.Point;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;

/**
 * Dashboard layout
 */
public class DashboardLayout extends ViewGroup
{
	private static final int MAX_ROWS = 64;

	/**
	 * Layout parameters
	 */
	public static class LayoutParams extends MarginLayoutParams
	{
		private int columnSpan;
		private int rowSpan;
		private int gravity;
		private Point size = new Point(-1, -1);

		public LayoutParams()
		{
			this(1, 1, Gravity.FILL);
		}

		public LayoutParams(int columnSpan, int rowSpan)
		{
			this(columnSpan, rowSpan, Gravity.FILL);
		}

		public LayoutParams(int columnSpan, int rowSpan, int gravity)
		{
			super(WRAP_CONTENT, WRAP_CONTENT);
			this.columnSpan = columnSpan;
			this.rowSpan = rowSpan;
			this.gravity = gravity;
		}

		public void setSpan(int columnSpan, int rowSpan)
		{
			this.columnSpan = columnSpan;
			this.rowSpan = rowSpan;
		}

		public int getColumnSpan()
		{
			return columnSpan;
		}

		public int getRowSpan()
		{
			return rowSpan;
		}

		public void setGravity(int gravity)
		{
			this.gravity = gravity;
		}

		public int getGravity()
		{
			return gravity;
		}

		public Point getSize()
		{
			return size;
		}

		public void setSize(Point size)
		{
			this.size = size;
		}
	}

	private int mMaxChildWidth = 0;
	private int mMaxChildHeight = 0;
	private int rowCount = 0;
	private int columnCount;
	private Map<View, Point> coordinates = new HashMap<View, Point>(0);


	/**
	 * @param context
	 */
	public DashboardLayout(Context context)
	{
		super(context);
	}

	/**
	 * @param context
	 * @param attrs
	 * @param defStyle
	 */
	public DashboardLayout(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}

	/**
	 * @param context
	 * @param attrs
	 */
	public DashboardLayout(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}
	
	/**
	 * Create logical grid
	 */
	private void createGrid()
	{
		final int childrenCount = getChildCount();
		coordinates = new HashMap<View, Point>(childrenCount);

		boolean[][] cellUsed = new boolean[MAX_ROWS][columnCount];
		int currentColumn = 0;
		int currentRow = 0;
		rowCount = 0;
		for(int i = 0; i < childrenCount; i++)
		{
			final View child = getChildAt(i);
			if (child.getVisibility() == GONE)
			{
				continue;
			}
			LayoutParams layoutParams = getLayoutParams(child);

			int columnSpan = layoutParams.getColumnSpan();
			if (columnSpan > columnCount)
			{
				throw new IllegalArgumentException("colSpan > number of columns");
			}

			int freeSpace = 0;
			for(int j = currentColumn; j < columnCount && freeSpace < columnSpan; j++)
			{
				if (!cellUsed[currentRow][j])
				{
					freeSpace++;
				}
				else
				{
					currentColumn++;
					freeSpace = 0;
				}
			}

			if (freeSpace < columnSpan)
			{
				currentRow++;
				currentColumn = 0;
			}
			for(int _c = 0; _c < columnSpan; _c++)
			{
				for(int _r = 0; _r < layoutParams.getRowSpan(); _r++)
				{
					cellUsed[currentRow + _r][currentColumn + _c] = true;
				}
			}
			coordinates.put(child, new Point(currentColumn, currentRow));
			currentColumn += columnSpan;
			if (currentColumn == columnCount)
			{
				currentColumn = 0;
				currentRow++;
			}
			rowCount = Math.max(currentRow + layoutParams.getRowSpan() - 1, rowCount);
		}
	}

	/* (non-Javadoc)
	 * @see android.view.View#onMeasure(int, int)
	 */
	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
	{
		createGrid();
		if (rowCount == 0)
		{
			setMeasuredDimension(widthMeasureSpec, heightMeasureSpec);
			return;
		}
		
		int width = MeasureSpec.getSize(widthMeasureSpec) - getPaddingLeft() - getPaddingRight();
		int height = MeasureSpec.getSize(heightMeasureSpec) - getPaddingTop() - getPaddingBottom();
				
		int columnSize = width / columnCount;
		int rowSize = height / rowCount;

		int count = getChildCount();
		for(int i = 0; i < count; i++)
		{
			final View view = getChildAt(i);
			if (view.getVisibility() == GONE)
				continue;
			Point point = coordinates.get(view);
			if (point == null)
				continue;
			LayoutParams layoutParams = getLayoutParams(view);
			int cw = layoutParams.getColumnSpan() * columnSize;
			int ch = layoutParams.getRowSpan() * rowSize;
			view.measure(MeasureSpec.makeMeasureSpec(cw, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(ch, MeasureSpec.EXACTLY));
		}

		setMeasuredDimension(resolveSize(width, widthMeasureSpec), resolveSize(height, heightMeasureSpec));
	}

	/* (non-Javadoc)
	 * @see android.view.ViewGroup#onLayout(boolean, int, int, int, int)
	 */
	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b)
	{
		if (rowCount == 0)
			return;
		
		int viewLeft  = l + getPaddingLeft();
		int viewRight = r - getPaddingRight();
		int viewTop = t + getPaddingTop();
		int viewBottom = b - getPaddingBottom();
		int width = viewRight - viewLeft;
		int height = viewBottom - viewTop;
		int columnSize = width / columnCount;
		int rowSize = height / rowCount;
		
		Iterator<Entry<View, Point>> iterator = coordinates.entrySet().iterator();
		while(iterator.hasNext())
		{
			Entry<View, Point> entry = iterator.next();
			View view = entry.getKey();
			Point point = entry.getValue();
			LayoutParams layoutParams = getLayoutParams(view);

			int cl = viewLeft + (point.x * columnSize);
			int ct = viewTop + (point.y * rowSize);
			int cr = viewLeft + ((point.x + layoutParams.getColumnSpan()) * columnSize);
			int cb = viewTop + ((point.y + layoutParams.getRowSpan()) * rowSize);
			view.layout(cl, ct, cr, cb);
		}
	}

	/**
	 * @param child
	 * @return
	 */
	private LayoutParams getLayoutParams(final View child)
	{
		return (LayoutParams)child.getLayoutParams();
	}

	/**
	 * @param count
	 */
	public void setColumnCount(int count)
	{
		columnCount = count;
	}
}
