package org.netxms.ui.android.main.activities;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.ComponentName;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuItem;
import android.view.SubMenu;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import org.netxms.base.GeoLocation;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.NXApplication;
import org.netxms.ui.android.R;
import org.netxms.ui.android.main.adapters.ObjectListAdapter;
import org.netxms.ui.android.main.fragments.AlarmBrowserFragment;
import org.netxms.ui.android.main.fragments.NodeInfoFragment;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.Stack;

/**
 * Node browser
 *
 * @author Victor Kirhenshtein
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 */

public class NodeBrowser extends AbstractClientActivity {
    private static final String TAG = NodeBrowser.class.getName();
    public static final int DUMMY_ROOT_OBJECT_ID = -100;
    private final Stack<AbstractObject> containerPath = new Stack<AbstractObject>();
    private ObjectListAdapter adapter;
    private int[] rootObjectFilter;
    private long initialParent;
    private AbstractObject currentParent = null;
    private long[] savedPath = null;
    private AbstractObject selectedObject = null;
    private ProgressDialog dialog;

    @Override
    protected void onCreateStep2(Bundle savedInstanceState) {
        dialog = new ProgressDialog(this);
        setContentView(R.layout.node_view);

        TextView title = findViewById(R.id.ScreenTitlePrimary);
        title.setText(R.string.nodes_title);

        rootObjectFilter = getIntent().getIntArrayExtra("rootObjectFilter");
        Log.d(TAG, "onCreateStep2: filters=" + Arrays.toString(rootObjectFilter));

        initialParent = DUMMY_ROOT_OBJECT_ID;
        currentParent = createDummyRoot();

        // keeps current list of nodes as datasource for listview
        adapter = new ObjectListAdapter(this);

        ListView listView = findViewById(R.id.NodeList);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener((parent, v, position, id) -> {
            AbstractObject obj = (AbstractObject) adapter.getItem(position);
            switch (obj.getObjectClass()) {
                case AbstractObject.OBJECT_CONTAINER:
                case AbstractObject.OBJECT_SUBNET:
                case AbstractObject.OBJECT_CLUSTER:
                case AbstractObject.OBJECT_ZONE:
                    containerPath.push(currentParent);
                    currentParent = obj;
                    refreshList();
                    break;
                case AbstractObject.OBJECT_NODE:
                case AbstractObject.OBJECT_MOBILEDEVICE:
                    showNodeInfo(obj.getObjectId());
                    break;
            }
        });

        registerForContextMenu(listView);

        // Restore saved state
        if (savedInstanceState != null)
            savedPath = savedInstanceState.getLongArray("currentPath");
    }

    private AbstractObject createDummyRoot() {
        return new AbstractObject(DUMMY_ROOT_OBJECT_ID, null) {
            @Override
            public AbstractObject[] getChildrenAsArray() {
                updateSessionAndChildren();
                return super.getChildrenAsArray();
            }

            @Override
            public long[] getChildIdList() {
                updateSessionAndChildren();
                return super.getChildIdList();
            }

            @Override
            public String getObjectName() {
                return "ROOT";
            }

            private void updateSessionAndChildren() {
                if (service != null) {
                    setSession(service.getSession());
                    if (children.isEmpty() && session != null) {
                        Set<Integer> filter = new HashSet<>();
                        for (int objectType : rootObjectFilter) {
                            filter.add(objectType);
                        }
                        AbstractObject[] topLevelObjects = session.getTopLevelObjects(filter);
                        for (AbstractObject topLevelObject : topLevelObjects) {
                            children.add(topLevelObject.getObjectId());
                        }
                    }
                }
            }
        };
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        savedPath = getFullPathAsId();
        outState.putLongArray("currentPath", savedPath);
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onResume() {
        super.onResume();
        NXApplication.activityResumed();
        if (service != null) {
            service.reconnect(false);
            rescanSavedPath();
        }
    }

    /**
     * @param objectId
     */
    public void showNodeInfo(long objectId) {
        Intent newIntent = new Intent(this, NodeInfoFragment.class);
        newIntent.putExtra("objectId", objectId);
        startActivity(newIntent);
    }

    @Override
    public void onServiceConnected(ComponentName name, IBinder binder) {
        super.onServiceConnected(name, binder);

        service.registerNodeBrowser(this);
        rescanSavedPath();
    }

    @Override
    public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
        android.view.MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.node_actions, menu);

        AdapterView.AdapterContextMenuInfo info = (AdapterContextMenuInfo) menuInfo;
        selectedObject = (AbstractObject) adapter.getItem(info.position);

        GeoLocation gl = selectedObject.getGeolocation();
        if ((gl == null) || (gl.getType() == GeoLocation.UNSET)) {
            hideMenuItem(menu, R.id.navigate_to);
        }

        if (selectedObject instanceof Node) {
            // add available tools to context menu
            List<ObjectTool> tools = service.getTools();
            if (tools != null) {
                SubMenu subMenu = menu.addSubMenu(Menu.NONE, 0, 0, getString(R.string.menu_tools));
                Iterator<ObjectTool> tl = tools.iterator();
                ObjectTool tool;
                while (tl.hasNext()) {
                    tool = tl.next();
                    switch (tool.getToolType()) {
                        case ObjectTool.TYPE_INTERNAL:
                        case ObjectTool.TYPE_ACTION:
                        case ObjectTool.TYPE_SERVER_COMMAND:
                        case ObjectTool.TYPE_SERVER_SCRIPT:
                            if (tool.isApplicableForObject(selectedObject))
                                subMenu.add(Menu.NONE, (int) tool.getId(), 0, tool.getDisplayName());
                            break;
                    }
                }
            }
        } else {
            hideMenuItem(menu, R.id.find_switch_port);
            hideMenuItem(menu, R.id.poll);
        }
    }

    /**
     * @param menu
     * @param id
     */
    private void hideMenuItem(ContextMenu menu, int id) {
        MenuItem item = menu.findItem(id);
        if (item != null)
            item.setVisible(false);
    }

    @Override
    public boolean onContextItemSelected(MenuItem item) {
        if (selectedObject == null)
            return super.onContextItemSelected(item);

        switch (item.getItemId()) {
            case R.id.find_switch_port:
                Intent fspIntent = new Intent(this, ConnectionPointBrowser.class);
                fspIntent.putExtra("nodeId", (int) selectedObject.getObjectId());
                startActivity(fspIntent);
                break;
            case R.id.view_alarms:
                new ViewAlarmsTask().execute((int) selectedObject.getObjectId());
                break;
            case R.id.unmanage:
                service.setObjectMgmtState(selectedObject.getObjectId(), false);
                refreshList();
                break;
            case R.id.manage:
                service.setObjectMgmtState(selectedObject.getObjectId(), true);
                refreshList();
                break;
            case R.id.poll_status:
                Intent psIntent = new Intent(this, NodePollerActivity.class);
                psIntent.putExtra("nodeId", (int) selectedObject.getObjectId());
                psIntent.putExtra("pollType", ObjectPollType.STATUS);
                startActivity(psIntent);
                break;
            case R.id.poll_configuration:
                Intent pcIntent = new Intent(this, NodePollerActivity.class);
                pcIntent.putExtra("nodeId", (int) selectedObject.getObjectId());
                pcIntent.putExtra("pollType", ObjectPollType.CONFIGURATION_NORMAL);
                startActivity(pcIntent);
                break;
            case R.id.poll_topology:
                Intent ptIntent = new Intent(this, NodePollerActivity.class);
                ptIntent.putExtra("nodeId", (int) selectedObject.getObjectId());
                ptIntent.putExtra("pollType", ObjectPollType.TOPOLOGY);
                startActivity(ptIntent);
                break;
            case R.id.poll_interfaces:
                Intent piIntent = new Intent(this, NodePollerActivity.class);
                piIntent.putExtra("nodeId", (int) selectedObject.getObjectId());
                piIntent.putExtra("pollType", ObjectPollType.INTERFACES);
                startActivity(piIntent);
                break;
            case R.id.navigate_to:
                GeoLocation gl = selectedObject.getGeolocation();
                Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("google.navigation:q=" + gl.getLatitude() + "," + gl.getLongitude()));
                intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET);
                try {
                    startActivity(intent);
                } catch (ActivityNotFoundException e) {
                    Toast.makeText(getApplicationContext(), "Navigation unavailable", Toast.LENGTH_LONG);
                }
                break;
            default:
                // if we didn't match static menu, check if it was some of tools
                List<ObjectTool> tools = service.getTools();
                if (tools != null) {
                    for (final ObjectTool tool : tools) {
                        if ((int) tool.getId() == item.getItemId()) {
                            if ((tool.getFlags() & ObjectTool.ASK_CONFIRMATION) != 0) {
                                String message = tool.getConfirmationText().replaceAll("%n", selectedObject.getObjectName()).replaceAll("%a", ((Node) selectedObject).getPrimaryIP().getHostAddress());
                                new AlertDialog.Builder(this).setIcon(android.R.drawable.ic_dialog_alert).setTitle(R.string.confirm_tool_execution).setMessage(message).setCancelable(true).setPositiveButton(R.string.yes, new OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialog, int which) {
                                        service.executeObjectTool(selectedObject.getObjectId(), tool);
                                    }
                                }).setNegativeButton(R.string.no, null).show();
                                break;
                            }
                            service.executeObjectTool(selectedObject.getObjectId(), tool);
                            break;
                        }
                    }
                }
                break;
        }

        return super.onContextItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        if (this.currentParent == null) {
            super.onBackPressed();
            return;
        }

        if (this.currentParent.getObjectId() == this.initialParent) {
            super.onBackPressed();
            return;
        }

        if (containerPath.empty()) {
            super.onBackPressed();
            return;
        }

        this.currentParent = containerPath.pop();
        this.refreshList();
    }

    /**
     * Refresh node list, force reload from server
     */
    public void refreshList() {
        if (currentParent == null) {
            currentParent = service.findObjectById(initialParent);
        }
        if (currentParent == null) {
            // if still null - problem with root node, stop loading
            return;
        }

        TextView curPath = findViewById(R.id.ScreenTitleSecondary);
        curPath.setText(getFullPath());
        new SyncMissingObjectsTask(currentParent.getObjectId()).execute(new Object[]{currentParent.getChildIdList()});
    }

    /**
     * Rescan saved path
     */
    private void rescanSavedPath() {
        // Restore to saved path if available
        if ((savedPath != null) && (savedPath.length > 0)) {
            containerPath.clear();
            for (int i = 0; i < savedPath.length - 1; i++) {
                final AbstractObject object;
                if (savedPath[i] == DUMMY_ROOT_OBJECT_ID) {
                    object = createDummyRoot();
                } else {
                    object = service.findObjectById(savedPath[i]);
                }
                if (object == null)
                    break;
                containerPath.push(object);
                Log.i(TAG, "object.getObjectId(): " + object.getObjectId());
            }
            currentParent = service.findObjectById(savedPath[savedPath.length - 1]);
            savedPath = null;
        }
        refreshList();
    }

    /**
     * Get full path to current position in object tree
     *
     * @return
     */
    private String getFullPath() {
        StringBuilder sb = new StringBuilder();
        if (containerPath.size() > 1) {
            for (AbstractObject o : containerPath.subList(1, containerPath.size())) {
                sb.append('/');
                sb.append(o.getObjectName());
            }
        }
        if (currentParent != null) {
            sb.append('/');
            if (currentParent.getObjectId() != DUMMY_ROOT_OBJECT_ID) {
                sb.append(currentParent.getObjectName());
            }
        }
        return sb.toString();
    }

    /**
     * Get full path to current position in object tree, as object identifiers
     *
     * @return
     */
    private long[] getFullPathAsId() {
        long[] path = new long[containerPath.size() + ((currentParent != null) ? 1 : 0)];
        int i = 0;
        for (AbstractObject o : containerPath)
            path[i++] = o.getObjectId();

        if (currentParent != null)
            path[i++] = currentParent.getObjectId();

        return path;
    }

    @Override
    protected void onDestroy() {
        service.registerNodeBrowser(null);
        super.onDestroy();
    }

    /**
     * Update node list, force refresh as necessary
     */
    public void updateNodeList() {
        if (adapter != null) {
            if (currentParent != null) {
                AbstractObject[] list = currentParent.getChildrenAsArray();
                if (list != null) {
                    adapter.setNodes(list);
                    adapter.notifyDataSetChanged();
                    return;
                }
            }
            refreshList();
        }
    }

    /**
     * @param nodeIdList
     */
    private void viewAlarms(ArrayList<Integer> nodeIdList) {
        Intent newIntent = new Intent(this, AlarmBrowserFragment.class);
        newIntent.putIntegerArrayListExtra("nodeIdList", nodeIdList);
        startActivity(newIntent);
    }

    /**
     * Internal task for synching missing objects
     */
    private class SyncMissingObjectsTask extends AsyncTask<Object, Void, AbstractObject[]> {
        private final long currentRoot;

        protected SyncMissingObjectsTask(long currentRoot) {
            this.currentRoot = currentRoot;
        }

        @Override
        protected void onPreExecute() {
            if (dialog != null) {
                dialog.setMessage(getString(R.string.progress_gathering_data));
                dialog.setIndeterminate(true);
                dialog.setCancelable(false);
                dialog.show();
            }
        }

        @Override
        protected AbstractObject[] doInBackground(Object... params) {
            try {
                service.getSession().syncMissingObjects((long[]) params[0], NXCSession.OBJECT_SYNC_WAIT);
                return currentParent.getChildrenAsArray();
            } catch (Exception e) {
                Log.e(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingObjectsTask", e);
            }
            return null;
        }

        @Override
        protected void onPostExecute(AbstractObject[] result) {
            if (dialog != null)
                dialog.cancel();
            if ((result != null) && (currentParent.getObjectId() == currentRoot)) {
                adapter.setNodes(result);
                adapter.notifyDataSetChanged();
            }
        }
    }

    /**
     * Internal task for synching missing objects
     */
    private class ViewAlarmsTask extends AsyncTask<Integer, Void, Integer> {
        private final ArrayList<Integer> childIdList = new ArrayList<>();

        @Override
        protected void onPreExecute() {
            if (dialog != null) {
                dialog.setMessage(getString(R.string.progress_gathering_data));
                dialog.setIndeterminate(true);
                dialog.setCancelable(false);
                dialog.show();
            }
        }

        protected void getChildsList(long[] list) {
            for (long l : list) {
                childIdList.add((int) l);
                AbstractObject obj = service.findObjectById(l);
                if (obj != null) {
                    int objectClass = obj.getObjectClass();
                    if (objectClass == AbstractObject.OBJECT_CONTAINER || objectClass == AbstractObject.OBJECT_CLUSTER) {
                        try {
                            long[] childIdList = obj.getChildIdList();
                            NXCSession session = service.getSession();
                            session.syncMissingObjects(childIdList, NXCSession.OBJECT_SYNC_WAIT);
                            getChildsList(childIdList);
                        } catch (Exception e) {
                            Log.e(TAG, "Exception while executing service.getSession().syncMissingObjects in SyncMissingChildsTask", e);
                        }
                    }
                }
            }
        }

        @Override
        protected Integer doInBackground(Integer... params) {
            long[] list = new long[params.length];
            for (int i = 0; i < params.length; i++)
                list[i] = params[i].longValue();
            getChildsList(list);
            return 0;
        }

        @Override
        protected void onPostExecute(Integer result) {
            if (dialog != null)
                dialog.cancel();
            viewAlarms(childIdList);
        }
    }
}
