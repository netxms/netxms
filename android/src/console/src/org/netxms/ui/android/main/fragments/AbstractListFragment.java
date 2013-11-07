package org.netxms.ui.android.main.fragments;

import org.netxms.ui.android.R;
import org.netxms.ui.android.main.activities.ConsolePreferences;
import org.netxms.ui.android.main.activities.HomeScreen;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.ListFragment;
import android.view.MenuItem;
import android.view.View;
import android.view.animation.AnimationUtils;
import android.widget.ListView;
import android.widget.TextView;

/**
 * Abstract base class for list fragments in the client.
 * Implements functionality for passing common parameters and
 * for handling common tasks.
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public abstract class AbstractListFragment extends ListFragment
{
	protected ClientConnectorService service = null;
	protected long nodeId;
	private View progressContainer;
	private View listContainer;
	private TextView standardEmptyView;
	protected boolean listShown;

	@Override
	public void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		setRetainInstance(true);
		setHasOptionsMenu(true);
	}

	@Override
	public boolean onContextItemSelected(MenuItem item)
	{
		if (defaultMenuAction(item))
			return true;
		return super.onContextItemSelected(item);
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item)
	{
		if (defaultMenuAction(item))
			return true;
		return super.onOptionsItemSelected(item);
	}

	@Override
	public void setListShown(boolean shown)
	{
		setListShown(shown, true);
	}

	@Override
	public void setListShownNoAnimation(boolean shown)
	{
		setListShown(shown, false);
	}

	public void setNodeId(long id)
	{
		nodeId = id;
	}

	public void setService(ClientConnectorService service)
	{
		this.service = service;
	}

	public abstract void refresh();

	public void setListShown(boolean shown, boolean animate)
	{
		if (listShown != shown)
		{
			listShown = shown;
			if (shown)
			{
				if (animate)
				{
					progressContainer.startAnimation(AnimationUtils.loadAnimation(getActivity(), android.R.anim.fade_out));
					listContainer.startAnimation(AnimationUtils.loadAnimation(getActivity(), android.R.anim.fade_in));
				}
				progressContainer.setVisibility(View.GONE);
				listContainer.setVisibility(View.VISIBLE);
			}
			else
			{
				if (animate)
				{
					progressContainer.startAnimation(AnimationUtils.loadAnimation(getActivity(), android.R.anim.fade_in));
					listContainer.startAnimation(AnimationUtils.loadAnimation(getActivity(), android.R.anim.fade_out));
				}
				progressContainer.setVisibility(View.VISIBLE);
				listContainer.setVisibility(View.INVISIBLE);
			}
		}
	}

	protected void createProgress(View v)
	{
		int INTERNAL_EMPTY_ID = 0x00ff0001;
		listContainer = v.findViewById(R.id.listContainer);
		progressContainer = v.findViewById(R.id.progressContainer);
		standardEmptyView = (TextView)v.findViewById(INTERNAL_EMPTY_ID);
		if (standardEmptyView != null)
			((ListView)v).setEmptyView(standardEmptyView);

		listShown = true;
	}

	private boolean defaultMenuAction(MenuItem item)
	{
		switch (item.getItemId())
		{
			case android.R.id.home:
				startActivity(new Intent(getActivity(), HomeScreen.class));
				return true;
			case R.id.settings:
				startActivity(new Intent(getActivity(), ConsolePreferences.class));
				return true;
		}
		return false;
	}
}
