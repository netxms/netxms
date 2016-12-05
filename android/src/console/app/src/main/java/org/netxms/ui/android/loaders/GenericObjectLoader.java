package org.netxms.ui.android.loaders;

import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.android.service.ClientConnectorService;

import android.content.Context;
import android.support.v4.content.AsyncTaskLoader;
import android.util.Log;

/**
 * Background loader for GenericObject objects. Notifies the fragment on job complete
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class GenericObjectLoader extends AsyncTaskLoader<AbstractObject>
{
	private static final String TAG = "nxclient/GenericObjectLoader";

	private ClientConnectorService service = null;
	private long nodeId = 0;
	private AbstractObject obj = null;

	/**
	 * @param context
	 */
	public GenericObjectLoader(Context context)
	{
		// Loaders may be used across multiple Activitys (assuming they aren't
		// bound to the LoaderManager), so NEVER hold a reference to the context
		// directly. Doing so will cause you to leak an entire Activity's context.
		// The superclass constructor will store a reference to the Application
		// Context instead, and can be retrieved with a call to getContext().
		super(context);
	}

	/**
	 * @param id
	 */
	public void setObjId(long id)
	{
		this.nodeId = id;
	}

	/**
	 * @param service
	 */
	public void setService(ClientConnectorService service)
	{
		this.service = service;
		if (service != null)
			service.registerGenericObjectLoader(this);
	}

	/* (non-Javadoc)
	 * @see android.support.v4.content.AsyncTaskLoader#loadInBackground()
	 */
	@Override
	public AbstractObject loadInBackground()
	{
		try
		{
			obj = null;
			if (service != null && service.getSession() != null)
			{
				service.getSession().syncObjectSet(new long[] { nodeId }, false, NXCSession.OBJECT_SYNC_WAIT);
				obj = service.getSession().findObjectById(nodeId);
			}
			else
				Log.d(TAG, "loadInBackground: service or session null!");
		}
		catch (Exception e)
		{
			Log.e(TAG, "Exception while executing loadInBackground", e);
		}
		return obj;
	}

	/* (non-Javadoc)
	 * @see android.support.v4.content.Loader#deliverResult(java.lang.Object)
	 */
	@Override
	public void deliverResult(AbstractObject newObj)
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
			if (newObj != null)
			{
				onReleaseResources(newObj);
				return;
			}
		}
		// Hold a reference to the old data so it doesn't get garbage collected.
		// The old data may still be in use (i.e. bound to an adapter, etc.), so
		// we must protect it until the new data has been delivered.
		AbstractObject oldObj = obj;
		obj = newObj;

		if (isStarted())
		{
			// If the Loader is in a started state, have the superclass deliver the
			// results to the client.
			super.deliverResult(newObj);
		}

		// Invalidate the old data as we don't need it any more.
		if (oldObj != null && oldObj != newObj)
		{
			Log.i(TAG, "Releasing any old data associated with this Loader.");
			onReleaseResources(oldObj);
		}
	}

	@Override
	protected void onStartLoading()
	{
		if (obj != null)
		{
			// Deliver any previously loaded data immediately.
			Log.i(TAG, "Delivering previously loaded data to the client...");
			deliverResult(obj);
		}

		// Register the observers that will notify the Loader when changes are made.
		if (service != null)
			service.registerGenericObjectLoader(this);

		if (takeContentChanged())
		{
			// When the observer detects a new installed application, it will call
			// onContentChanged() on the Loader, which will cause the next call to
			// takeContentChanged() to return true. If this is ever the case (or if
			// the current data is null), we force a new load.
			Log.i(TAG, "A content change has been detected... so force load!");
			forceLoad();
		}
		else if (obj == null)
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

	/* (non-Javadoc)
	 * @see android.support.v4.content.Loader#onReset()
	 */
	@Override
	protected void onReset()
	{
		// Ensure the loader is stopped.
		onStopLoading();

		// At this point we can release the resources associated with 'apps'.
		if (obj != null)
		{
			onReleaseResources(obj);
			obj = null;
		}

		// The Loader is being reset, so we should stop monitoring for changes.
		if (service != null)
			service.unregisterGenericObjectLoader(this);
	}

	/* (non-Javadoc)
	 * @see android.support.v4.content.AsyncTaskLoader#onCanceled(java.lang.Object)
	 */
	@Override
	public void onCanceled(AbstractObject o)
	{
		// Attempt to cancel the current asynchronous load.
		super.onCanceled(o);

		// The load has been canceled, so we should release the resources
		// associated with 'obj'.
		onReleaseResources(o);
	}

	/* (non-Javadoc)
	 * @see android.support.v4.content.Loader#forceLoad()
	 */
	@Override
	public void forceLoad()
	{
		super.forceLoad();
	}

	/**
	 * Helper method to take care of releasing resources associated with an
	 * actively loaded data set.
	 */
	protected void onReleaseResources(AbstractObject o)
	{
		// For a simple List, there is nothing to do. For something like a Cursor,
		// we would close it in this method. All resources associated with the
		// Loader should be released here.
	}
}
