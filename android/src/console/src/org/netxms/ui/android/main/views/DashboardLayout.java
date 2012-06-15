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

public class DashboardLayout extends ViewGroup
{
	private static final int MAX_ROWS = 64;

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
	private int columnCount;

	public DashboardLayout(Context context)
	{
		super(context);
	}

	public DashboardLayout(Context context, AttributeSet attrs, int defStyle)
	{
		super(context, attrs, defStyle);
	}

	public DashboardLayout(Context context, AttributeSet attrs)
	{
		super(context, attrs);
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec)
	{
		mMaxChildWidth = 0;
		mMaxChildHeight = 0;

		final int childWidthMeasureSpec = MeasureSpec.makeMeasureSpec(MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.AT_MOST);
		final int childHeightMeasureSpec = MeasureSpec.makeMeasureSpec(MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.AT_MOST);

		int count = getChildCount();
		for(int i = 0; i < count; i++)
		{
			final View child = getChildAt(i);
			if (child.getVisibility() != GONE)
			{
				child.measure(childWidthMeasureSpec, childHeightMeasureSpec);
				mMaxChildWidth = Math.max(mMaxChildWidth, child.getMeasuredWidth());
				mMaxChildHeight = Math.max(mMaxChildHeight, child.getMeasuredHeight());
				Log.d("layout", "mMaxChildWidth=" + mMaxChildWidth);
				Log.d("layout", "mMaxChildHeight=" + mMaxChildHeight);
			}
		}

		setMeasuredDimension(resolveSize(mMaxChildWidth, widthMeasureSpec), resolveSize(mMaxChildHeight, heightMeasureSpec));
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b)
	{
		final int childrenCount = getChildCount();

		Map<View, Point> coordinates = new HashMap<View, Point>(childrenCount);

		boolean[][] cellUsed = new boolean[columnCount][MAX_ROWS];
		int currentColumn = 0;
		int currentRow = 0;
		int rowCount = 0;
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

		int width = r - l;
		int height = b - t;
		int columnSize = width / columnCount;
		int rowSize = height / rowCount;

		Iterator<Entry<View, Point>> iterator = coordinates.entrySet().iterator();
		while(iterator.hasNext())
		{
			Entry<View, Point> entry = iterator.next();
			View view = entry.getKey();
			Point point = entry.getValue();
			LayoutParams layoutParams = getLayoutParams(view);

			int cl = l + (point.x * columnSize);
			int ct = t + (point.y * rowSize);
			int cr = l + ((point.x + layoutParams.getColumnSpan()) * columnSize);
			int cb = t + ((point.y + layoutParams.getRowSpan()) * rowSize);
			view.layout(cl, ct, cr, cb);
		}
	}

	private LayoutParams getLayoutParams(final View child)
	{
		return (LayoutParams)child.getLayoutParams();
	}

	public void setColumnCount(int count)
	{
		columnCount = count;
	}
}
