/**
 * 
 */
package org.netxms.ui.android.main.activities;

import java.util.Arrays;
import org.netxms.client.Table;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.android.R;
import android.content.ComponentName;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.IBinder;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.widget.TextView;

/**
 * Last value for table DCI
 */
public class TableLastValues extends AbstractClientActivity
{
	private static final String TAG = "nxclient/TableLastValues";
	
	private long nodeId;
	private long dciId;
	private TextView textView;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		nodeId = getIntent().getIntExtra("nodeId", 0);
		dciId = getIntent().getIntExtra("dciId", 0);
		
		setContentView(R.layout.table_last_values);

		textView = (TextView)findViewById(R.id.TextView);
		textView.setTypeface(Typeface.MONOSPACE);
		textView.setMovementMethod(new ScrollingMovementMethod());
		textView.setHorizontallyScrolling(true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onServiceConnected(android.content.ComponentName, android.os.IBinder)
	 */
	@Override
	public void onServiceConnected(ComponentName name, IBinder binder)
	{
		super.onServiceConnected(name, binder);
		GenericObject object = service.findObjectById(nodeId);
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(((object != null) ? object.getObjectName() : ("[" + Long.toString(nodeId) + "]")) + ":" + getIntent().getStringExtra("description"));
		refresh();
	}
	
	/**
	 * Refresh view
	 */
	private void refresh()
	{
		final Thread thread = new Thread() {
			@Override
			public void run()
			{
				try
				{
					final Table table = service.getSession().getTableLastValues(nodeId, dciId);
					runOnUiThread(new Runnable() {
						@Override
						public void run()
						{
							showTable(table);
						}
					});
				}
				catch(Exception e)
				{
					Log.e(TAG, "Exception in worker thread", e);
				}
			}
		};
		thread.setDaemon(true);
		thread.start();
	}
	
	/**
	 * @param table
	 */
	private void showTable(Table table)
	{
		// calculate column widths
		int[] widths = new int[table.getColumnCount()];
		Arrays.fill(widths, 1);
		for(int i = 0; i < table.getRowCount(); i++)
		{
			for(int j = 0; j < table.getColumnCount(); j++)
			{
				int len = table.getCell(i, j).length();
				if (len > widths[j])
					widths[j] = len;
			}
		}
		
		textView.setText("");
		for(int i = 0; i < table.getRowCount(); i++)
		{
			StringBuilder sb = new StringBuilder();
			for(int j = 0; j < table.getColumnCount(); j++)
			{
				if (j > 0)
					sb.append(" | ");
				String value = table.getCell(i, j);
				sb.append(value);
				for(int k = value.length(); k < widths[j]; k++)
					sb.append(' ');
			}
			sb.append('\n');
			textView.append(sb.toString());
		}
	}
}
