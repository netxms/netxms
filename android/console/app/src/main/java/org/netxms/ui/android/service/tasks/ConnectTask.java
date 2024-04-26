/**
 *
 */
package org.netxms.ui.android.service.tasks;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.WindowManager;

import org.netxms.base.VersionInfo;
import org.netxms.client.NXCSession;
import org.netxms.client.ProtocolVersion;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;
import org.netxms.ui.android.service.ClientConnectorService.ConnectionStatus;

/**
 * Connect task
 */
public class ConnectTask extends Thread {
    private static final String TAG = "nxclient/ConnectTask";

    private final ClientConnectorService service;
    private String server;
    private Integer port;
    private String login;
    private String password;
    private boolean forceReconnect;

    /**
     * @param service
     */
    public ConnectTask(ClientConnectorService service) {
        this.service = service;
    }

    /**
     * Execute task
     *
     * @param server
     * @param port
     * @param login
     * @param password
     * @param forceReconnect
     */
    public void execute(String server, Integer port, String login, String password, boolean forceReconnect) {
        this.server = server;
        this.port = port;
        this.login = login;
        this.password = password;
        this.forceReconnect = forceReconnect;
        start();
    }

    /*
     * (non-Javadoc)
     *
     * @see java.lang.Thread#run()
     */
    @Override
    public void run() {
        if (service != null) {
            if (isInternetOn()) {
                service.setConnectionStatus(ConnectionStatus.CS_INPROGRESS, "");
                NXCSession session = service.getSession();
                if (session != null) {
                    if (forceReconnect) {
                        session.disconnect();
                        session = null;
                    } else {
                        if (session.checkConnection()) {
                            service.setConnectionStatus(ConnectionStatus.CS_ALREADYCONNECTED, session.getServerAddress());
                        } else {
                            Log.e(TAG, "session.checkConnection() failed");
                            service.onError("Connection closed");
                            session = null;
                        }
                    }
                }
                if (session == null) // Already null or invalidated
                {
                    DisplayMetrics metrics = new DisplayMetrics();
                    WindowManager wm = (WindowManager) service.getSystemService(Context.WINDOW_SERVICE);
                    wm.getDefaultDisplay().getMetrics(metrics);

                    session = new NXCSession(server, port, true);
                    session.setClientInfo(String.format("nxmc-android/%s.%s", VersionInfo.version(), service.getString(R.string.build_number)));
                    session.setClientType((metrics.densityDpi >= DisplayMetrics.DENSITY_HIGH) ? NXCSession.TABLET_CLIENT : NXCSession.MOBILE_CLIENT);
                    try {
                        Log.d(TAG, "calling session.connect()");
                        session.connect(new int[]{ProtocolVersion.INDEX_FULL});
                        Log.d(TAG, "calling session.login()");
                        session.login(login, password);
                        Log.d(TAG, "calling session.subscribe()");
                        session.subscribe(NXCSession.CHANNEL_ALARMS);
                        session.subscribe(NXCSession.CHANNEL_OBJECTS);
                        session.syncObjects(); // TODO: remove full sync
                        service.onConnect(session, session.getAlarms());
                        service.loadTools();
                    } catch (Exception e) {
                        Log.e(TAG, "Exception on connect attempt", e);
                        service.onError(e.getLocalizedMessage());
                    }
                }
            } else {
                Log.w(TAG, "No internet connection");
                service.setConnectionStatus(ConnectionStatus.CS_NOCONNECTION, "");
            }
        } else {
            Log.w(TAG, "Service unavailable");
        }
    }

    /**
     * Check for internet connectivity
     */
    private boolean isInternetOn() {
        ConnectivityManager connec = (ConnectivityManager) service.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connec != null) {
            NetworkInfo wifi = connec.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
            if (wifi != null && wifi.getState() == NetworkInfo.State.CONNECTED)
                return true;
            NetworkInfo mobile = connec.getNetworkInfo(ConnectivityManager.TYPE_MOBILE);
            return mobile != null && mobile.getState() == NetworkInfo.State.CONNECTED;
        }
        return false;
    }

}
