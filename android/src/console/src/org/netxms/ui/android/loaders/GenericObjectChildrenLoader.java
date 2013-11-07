package org.netxms.ui.android.loaders;

import java.util.Set;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Context;
import android.support.v4.content.AsyncTaskLoader;
import android.util.Log;

/**
 * Background loader for a set of GenericObjects, such as list of interfaces.
 * Notifies the fragment on job complete
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class GenericObjectChildrenLoader extends AsyncTaskLoader<Set<AbstractObject>>
{
	private static final String TAG = "nxclient/GenericObjectChildrenLoader";

	private ClientConnectorService service = null;
	private long objId = 0;
	private int classFilter = AbstractObject.OBJECT_INTERFACE;
	private Set<AbstractObject> children = null;

	public GenericObjectChildrenLoader(Context context)
	{
		// Loaders may be used across multiple Activitys (assuming they aren't
		// bound to the LoaderManager), so NEVER hold a reference to the context
		// directly. Doing so will cause you to leak an entire Activity's context.
		// The superclass constructor will store a reference to the Application
		// Context instead, and can be retrieved with a call to getContext().
		super(context);
	}

	public void setObjId(long id)
	{
		this.objId = id;
	}

	public void setClassFilter(int classFilter)
	{
		this.classFilter = classFilter;
	}

	public void setService(ClientConnectorService service)
	{
		this.service = service;
		if (service != null)
			service.registerGenericObjectChildrenLoader(this);
	}

	@Override
	public Set<AbstractObject> loadInBackground()
	{
		try
		{
			children = null;
			if (service != null && service.getSession() != null)
			{
				service.getSession().syncObjectSet(new long[] { objId }, false, NXCSession.OBJECT_SYNC_WAIT);
				AbstractObject go = service.getSession().findObjectById(objId);
				if (go != null)
				{
					service.getSession().syncMissingObjects(go.getChildIdList(), false, NXCSession.OBJECT_SYNC_WAIT);
					children = go.getAllChilds(classFilter);
				}
			}
			else
				Log.d(TAG, "loadInBackground: service or session null!");
		}
		catch (Exception e)
		{
			Log.w(TAG, "Exception while executing loadInBackground", e);
		}
		return children;
	}

	@Override
	public void deliverResult(Set<AbstractObject> newChildren)
	{
		if (isReset())
		{
			Log.w(TAG, "Warning! An async query came in while the Loader was reset!");
			// The Loader has been reset; ignore the result and invalidate the data.
			// This can happen when the Loader is reset while an asynchronous query
			// is working in the background. That is, when the background thread
			// finishes its work and attempts to deliver the results to the client,
			// it will see here that the Loader has been reset and discard any
			// resources associated with the new data as necessary.
			if (newChildren != null)
			{
				onReleaseResources(newChildren);
				return;
			}
		}
		// Hold a reference to the old data so it doesn't get garbage collected.
		// The old data may still be in use (i.e. bound to an adapter, etc.), so
		// we must protect it until the new data has been delivered.
		Set<AbstractObject> oldChildren = children;
		children = newChildren;

		if (isStarted())
		{
			// If the Loader is in a started state, have the superclass deliver the
			// results to the client.
			super.deliverResult(newChildren);
		}

		// Invalidate the old data as we don't need it any more.
		if (oldChildren != null && oldChildren != newChildren)
		{
			Log.i(TAG, "Releasing any old data associated with this Loader.");
			onReleaseResources(oldChildren);
		}
	}

	@Override
	protected void onStartLoading()
	{
		if (children != null)
		{
			// Deliver any previously loaded data immediately.
			Log.i(TAG, "Delivering previously loaded data to the client...");
			deliverResult(children);
		}

		// Register the observers that will notify the Loader when changes are made.
		if (service != null)
			service.registerGenericObjectChildrenLoader(this);

		if (takeContentChanged())
		{
			// When the observer detects a new installed application, it will call
			// onContentChanged() on the Loader, which will cause the next call to
			// takeContentChanged() to return true. If this is ever the case (or if
			// the current data is null), we force a new load.
			Log.i(TAG, "A content change has been detected... so force load!");
			forceLoad();
		}
		else if (children == null)
		{
			// If the current data is null... then we should make it non-null! :)
			Log.i(TAG, "The current data is null... so force load!");
			forceLoad();
		}
	}

	@Override
	protected void onStopLoading()
	{
		// The Loader has been put in a stopped state, so we should attempt to
		// cancel the current load (if there is one).
		cancelLoad();

		// Note that we leave the observer as is; Loaders in a stopped state
		// should still monitor the data source for changes so that the Loader
		// will know to force a new load if it is ever started again.
	}

	@Override
	protected void onReset()
	{
		// Ensure the loader is stopped.
		onStopLoading();

		// At this point we can release the resources associated with 'apps'.
		if (children != null)
		{
			onReleaseResources(children);
			children = null;
		}

		// The Loader is being reset, so we should stop monitoring for changes.
		if (service != null)
			service.unregisterGenericObjectChildrenLoader(this);
	}

	@Override
	public void onCanceled(Set<AbstractObject> o)
	{
		// Attempt to cancel the current asynchronous load.
		super.onCanceled(o);

		// The load has been canceled, so we should release the resources
		// associated with 'obj'.
		onReleaseResources(o);
	}

	@Override
	public void forceLoad()
	{
		super.forceLoad();
	}

	/**
	 * Helper method to take care of releasing resources associated with an
	 * actively loaded data set.
	 */
	protected void onReleaseResources(Set<AbstractObject> o)
	{
		// For a simple List, there is nothing to do. For something like a Cursor,
		// we would close it in this method. All resources associated with the
		// Loader should be released here.
	}
}
