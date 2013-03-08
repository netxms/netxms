/**
 * 
 */
package org.netxms.ui.android.main.fragments;

import org.netxms.client.objects.GenericObject;
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

public class OverviewFragment extends AbstractListFragment implements LoaderManager.LoaderCallbacks<GenericObject>
{
	private OverviewAdapter adapter = null;
	private GenericObjectLoader loader = null;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container,
			Bundle savedInstanceState)
	{
		View v = inflater.inflate(R.layout.overview_fragment, container, false);
		createProgress(v);
		return v;
	}

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

	@Override
	public Loader<GenericObject> onCreateLoader(int arg0, Bundle arg1)
	{
		return new GenericObjectLoader(getActivity());
	}

	@Override
	public void onLoadFinished(Loader<GenericObject> arg0, GenericObject arg1)
	{
		setListShown(true, true);
		if (adapter != null)
		{
			adapter.setValues(arg1);
			adapter.notifyDataSetChanged();
		}
	}

	@Override
	public void onLoaderReset(Loader<GenericObject> arg0)
	{
	}
}
