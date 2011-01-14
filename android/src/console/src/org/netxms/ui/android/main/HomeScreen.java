package org.netxms.ui.android.main;

import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

public class HomeScreen extends Activity
{
	private ClientConnectorService service;
	
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.homescreen);

		startService(new Intent(this, ClientConnectorService.class));
		
		/*
		bindService(new Intent(), new ServiceConnection() {
			
		}, flags);*/
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onCreateOptionsMenu(android.view.Menu)
	 */
	@Override
	public boolean onCreateOptionsMenu(Menu menu)
	{
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.main_menu, menu);
	    return true;
	}

	/* (non-Javadoc)
	 * @see android.app.Activity#onOptionsItemSelected(android.view.MenuItem)
	 */
	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		switch(item.getItemId())
		{
			case R.id.settings:
				startActivity(new Intent(this, ConsolePreferences.class));
				return true;
			default:
				return super.onOptionsItemSelected(item);
		}
	}
}
