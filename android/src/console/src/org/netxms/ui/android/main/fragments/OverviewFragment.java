/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.R;
import org.netxms.ui.android.loaders.GenericObjectLoader;
import org.netxms.ui.android.main.adapters.OverviewAdapter;

import android.os.Bundle;
import android.support.v4.app.LoaderManager;
import android.support.v4.content.Loader;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

/**
 * Fragment for overview info
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 * 
 */

public class OverviewFragment extends AbstractListFragment implements LoaderManager.LoaderCallbacks<AbstractObject>
{
	private OverviewAdapter adapter = null;
	private GenericObjectLoader loader = null;

	/* (non-Javadoc)
	 * @see android.support.v4.app.ListFragment#onCreateView(android.view.LayoutInflater, android.view.ViewGroup, android.os.Bundle)
	 */
	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState)
	{
		View v = inflater.inflate(R.layout.overview_fragment, container, false);
		createProgress(v);
		return v;
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.Fragment#onActivityCreated(android.os.Bundle)
	 */
	@Override
	public void onActivityCreated(Bundle savedInstanceState)
	{
		super.onActivityCreated(savedInstanceState);
		adapter = new OverviewAdapter(getActivity());
		setListAdapter(adapter);
		setListShown(false, true);
		loader = (GenericObjectLoader)getActivity().getSupportLoaderManager().initLoader(R.layout.overview_fragment, null, this);
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setService(service);
		}
	}

	@Override
	public void refresh()
	{
		if (loader != null)
		{
			loader.setObjId(nodeId);
			loader.setService(service);
			loader.forceLoad();
		}
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onCreateLoader(int, android.os.Bundle)
	 */
	@Override
	public Loader<AbstractObject> onCreateLoader(int arg0, Bundle arg1)
	{
		return new GenericObjectLoader(getActivity());
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onLoadFinished(android.support.v4.content.Loader, java.lang.Object)
	 */
	@Override
	public void onLoadFinished(Loader<AbstractObject> arg0, AbstractObject arg1)
	{
		setListShown(true, true);
		if (adapter != null)
		{
			adapter.setValues(arg1);
			adapter.notifyDataSetChanged();
		}
	}

	/* (non-Javadoc)
	 * @see android.support.v4.app.LoaderManager.LoaderCallbacks#onLoaderReset(android.support.v4.content.Loader)
	 */
	@Override
	public void onLoaderReset(Loader<AbstractObject> arg0)
	{
	}
}
