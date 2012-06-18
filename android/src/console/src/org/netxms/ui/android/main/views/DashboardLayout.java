package org.netxms.ui.android.main.views;

import java.util.Arrays;
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

		/**
		 * Create default layout parameters - 1x1 span and gravity FILL
		 */
		public LayoutParams()
		{
			this(1, 1, Gravity.FILL);
		}

		/**
		 * Create FILL layout with given row and column span
		 * 
		 * @param columnSpan
		 * @param rowSpan
		 */
		public LayoutParams(int columnSpan, int rowSpan)
		{
			this(columnSpan, rowSpan, Gravity.FILL);
		}

		/**
		 * @param columnSpan
		 * @param rowSpan
		 * @param gravity
		 */
		public LayoutParams(int columnSpan, int rowSpan, int gravity)
		{
			super(WRAP_CONTENT, WRAP_CONTENT);
			this.columnSpan = columnSpan;
			this.rowSpan = rowSpan;
			this.gravity = gravity;
		}

		/**
		 * @param columnSpan
		 * @param rowSpan
		 */
		public void setSpan(int columnSpan, int rowSpan)
		{
			this.columnSpan = columnSpan;
			this.rowSpan = rowSpan;
		}

		/**
		 * @return
		 */
		public int getColumnSpan()
		{
			return columnSpan;
		}

		/**
		 * @return
		 */
		public int getRowSpan()
		{
			return rowSpan;
		}

		/**
		 * @param gravity
		 */
		public void setGravity(int gravity)
		{
			this.gravity = gravity;
		}

		/**
		 * @return
		 */
		public int getGravity()
		{
			return gravity;
		}

		/**
		 * @return
		 */
		public Point getSize()
		{
			return size;
		}

		/**
		 * @param size
		 */
		public void setSize(Point size)
		{
			this.size = size;
		}
	}

	private int rowCount = 0;
	private int columnCount;
	private Map<View, Point> coordinates = new HashMap<View, Point>(0);
	private int[] rowStart = null;

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
		
		// Calculate size for each row
		int[] rowSize = new int[rowCount];
		Arrays.fill(rowSize, 0);
		boolean[] rowFill = new boolean[rowCount];
		Arrays.fill(rowFill, false);
		
		for(Entry<View, Point> e : coordinates.entrySet())
		{
			LayoutParams layoutParams = getLayoutParams(e.getKey());
			if ((layoutParams.gravity & Gravity.VERTICAL_GRAVITY_MASK) == Gravity.FILL_VERTICAL)
			{
				if (layoutParams.rowSpan == 1)
				{
					rowFill[e.getValue().y] = true;
				}
			}
			else
			{
				e.getKey().measure(MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED), MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED));
				rowSize[e.getValue().y] = Math.max(rowSize[e.getValue().y], e.getKey().getMeasuredHeight());
			}
		}
		
		int usedSpace = 0;
		int fillRows = 0;
		for(int i = 0; i < rowCount; i++)
		{
			if (rowFill[i])
				fillRows++;
			usedSpace += rowSize[i];
		}
		
		// If children requires too much space, reduce biggest
		if (usedSpace > height)
		{
			int extraSpace = usedSpace - height;
			int maxRowSize = height / rowCount;
			for(int i = 0; i < rowCount; i++)
			{
				if (rowSize[i] > maxRowSize)
				{
					int delta = Math.min(rowSize[i] - maxRowSize, extraSpace);
					rowSize[i] -= delta;
					extraSpace -= delta;
					usedSpace -= delta;
				}
			}
		}
		
		// distribute space evenly between fill rows
		if (fillRows > 0)
		{
			int size = (height - usedSpace) / fillRows;
			for(int i = 0; i < rowCount; i++)
			{
				if (rowFill[i])
					rowSize[i] += size;
			}
		}

		// calculate row starts
		rowStart = new int[rowCount];
		rowStart[0] = 0;
		for(int i = 1; i < rowCount; i++)
			rowStart[i] = rowStart[i - 1] + rowSize[i - 1];
		
		int columnSize = width / columnCount;

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
			
			Point p = coordinates.get(view);	
			int ch = rowSize[p.y];
			for(int j = 1; j < layoutParams.getRowSpan(); j++)
				ch += rowSize[p.y + j];
			view.measure(MeasureSpec.makeMeasureSpec(cw, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(ch, MeasureSpec.EXACTLY));
		}

		setMeasuredDimension(resolveSize(MeasureSpec.getSize(widthMeasureSpec), widthMeasureSpec), resolveSize(MeasureSpec.getSize(heightMeasureSpec), heightMeasureSpec));
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
		int width = viewRight - viewLeft;
		int columnSize = width / columnCount;
		
		Iterator<Entry<View, Point>> iterator = coordinates.entrySet().iterator();
		while(iterator.hasNext())
		{
			Entry<View, Point> entry = iterator.next();
			View view = entry.getKey();
			Point point = entry.getValue();

			int cl = viewLeft + (point.x * columnSize);
			int ct = viewTop + rowStart[point.y];
			int cr = viewLeft + cl + view.getMeasuredWidth() - 1;
			int cb = viewTop + ct + view.getMeasuredHeight() - 1;
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
