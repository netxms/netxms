/**
 * 
 */
package org.netxms.ui.android.main.activities;

import android.content.ComponentName;
import android.graphics.Color;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.IBinder;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.method.ScrollingMovementMethod;
import android.text.style.ForegroundColorSpan;
import android.util.Log;
import android.widget.TextView;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.NodePollType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.R;

/**
 * Node poller activity
 */
public class NodePollerActivity extends AbstractClientActivity
{
	private static final String TAG = "nxclient/NodePoller";
	private static final String[] POLL_NAME = { "", "Status", "Configuration (full)", "Interface", "Topology", "Configuration", "Instance" };
	
	private long nodeId;
	private NodePollType pollType;
	private boolean pollInProgress = false;
	private TextView textView;
	
	/* (non-Javadoc)
	 * @see org.netxms.ui.android.main.activities.AbstractClientActivity#onCreateStep2(android.os.Bundle)
	 */
	@Override
	protected void onCreateStep2(Bundle savedInstanceState)
	{
		nodeId = getIntent().getIntExtra("nodeId", 0);
		pollType = (NodePollType)getIntent().getSerializableExtra("pollType");
		
		setContentView(R.layout.node_poller);
		
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
		AbstractObject object = service.findObjectById(nodeId);
		TextView title = (TextView)findViewById(R.id.ScreenTitlePrimary);
		title.setText(POLL_NAME[pollType.getValue()] + " poll: " + ((object != null) ? object.getObjectName() : ("[" + Long.toString(nodeId) + "]")));
		restart();
	}

	/**
	 * Restart poll
	 */
	private void restart()
	{
		if (pollInProgress)
			return;
		
		pollInProgress = true;
		addPollerMessage("\u007Fl**** Poll request sent to server ****\n");
		final Thread thread = new Thread() {
			@Override
			public void run()
			{
				try
				{
					service.getSession().pollNode(nodeId, pollType, new TextOutputListener() {
						@Override
						public void messageReceived(final String text)
						{
							runOnUiThread(new Runnable() {
								@Override
								public void run()
								{
									addPollerMessage(text);
								}
							});
						}

						@Override
						public void setStreamId(long l)

						{
						}
					});
					onPollCompleted(true, null);
				}
				catch(final Exception e)
				{
					Log.e(TAG, "Exception in worker thread", e);
					onPollCompleted(false, e.getLocalizedMessage());
				}
				pollInProgress = false;
			}
		};
		thread.setDaemon(true);
		thread.start();
	}
	
	/**
	 * Poll completion handler. Called from worker thread.
	 * 
	 * @param success
	 * @param errorMessage
	 */
	private void onPollCompleted(final boolean success, final String errorMessage)
	{
		runOnUiThread(new Runnable() {
			@Override
			public void run()
			{
				if (success)
				{
					addPollerMessage("\u007Fl**** Poll completed successfully ****\n\n");
				}
				else
				{
					addPollerMessage("\u007FePOLL ERROR: " + errorMessage + "\n");
					addPollerMessage("\u007Fl**** Poll failed ****\n\n");
				}
			}
		});
	}
	
	/**
	 * Add poller message to text area. Must be called from UI thread.
	 * 
	 * @param message poller message
	 */
	private void addPollerMessage(String message)
	{
		//Date now = new Date();
		//textView.append("[" + DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM).format(now) + "] ");
		
		int index = message.indexOf(0x7F);
		if (index != -1)
		{
			textView.append(message.substring(0, index));
			char code = message.charAt(index + 1);
			
			final SpannableString ssb = new SpannableString(message.substring(index + 2));
			ssb.setSpan(new ForegroundColorSpan(getTextColor(code)), 0, ssb.length(), Spannable.SPAN_EXCLUSIVE_EXCLUSIVE);
			textView.append(ssb);
		}
		else
		{
			textView.append(message);
		}
		
		final Layout layout = textView.getLayout();
      if(layout != null)
      {
      	int scrollDelta = layout.getLineBottom(textView.getLineCount() - 1) - textView.getScrollY() - textView.getHeight();
      	if(scrollDelta > 0)
      		textView.scrollBy(0, scrollDelta);
      }
	}

	/**
	 * @param code
	 * @return
	 */
	private int getTextColor(int code)
	{
		switch(code)
		{
			case 'e':
				return 0xFFC00000;
			case 'w':
				return 0xFFFF8000;
			case 'i':
				return 0xFF008000;
			case 'l':
				return 0xFF0000C0;
		}
		return Color.BLACK;
	}
}
