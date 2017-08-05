package com.rfelements.workers;

import java.util.Timer;
import java.util.TimerTask;
import org.netxms.bridge.Platform;
import com.rfelements.DeviceType;
import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.config.Settings;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.DeviceCredentials;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.rest.Rest;

/**
 * @author Pichanič Ján
 */
public class SingleWorker extends Timer {

    private final DeviceCredentials deviceCredentials;

    private final DeviceType type;

    private Cache cache = CacheImpl.getInstance();

    public SingleWorker(DeviceCredentials deviceCredentials, DeviceType type) {
        super(true);
        this.deviceCredentials = deviceCredentials;
        this.type = type;
        Platform.writeDebugLog(3, Thread.currentThread().getName() + " [SingleWorker] Construction ...");
        Platform.writeDebugLog(3,
                Thread.currentThread().getName() + " [SingleWorker] Args - deviceCredentials : " + deviceCredentials.hashCode() + ", type : " + type.toString());
    }

    public void start() {
        this.scheduleAtFixedRate(new TimerTask() {

            @Override
            public void run() {
                Platform.writeDebugLog(3, Thread.currentThread().getName() +
                        " [SingleWorker] Reschedule, EntryPoint : " + type.toString() + "  URL : " + deviceCredentials.getUrl() + " , deviceCredentials : " + deviceCredentials.hashCode());
                switch (type) {
                    case LIGOWAVE_AP:
                        updateLigowave();
                        break;
                    case UBIQUITI_AP:
                        updateUbiquiti();
                        break;
                    case LIGOWAVE_CLIENT:
                        updateLigowave();
                        break;
                    case UBIQUITI_CLIENT:
                        updateUbiquiti();
                        break;
                    default:
                        break;
                }
            }
        }, 0, Settings.UPDATE_PERIOD);
    }

    private void updateUbiquiti() {
        Platform.writeDebugLog(3, Thread.currentThread().getName() +
                " [SingleWorker] Calling update ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl() + " , deviceCredentials : "
                + deviceCredentials.hashCode());
        try {
            Ubiquiti ubnt = Rest.updateUbiquitiJsonObject(deviceCredentials, Settings.LOGIN_TRIES_COUNT);
            cache.putJsonObject(deviceCredentials.getIp(), ubnt);

            // DEBUG OUTPUT
            if (ubnt == null)
                Platform.writeDebugLog(3, Thread.currentThread().getName() +
                        " [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl() + " JSON obj : null");
            else
                Platform.writeDebugLog(3, Thread.currentThread().getName() +
                        " [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl() + " JSON obj : " + ubnt.getClass().getName());
        } catch (CollectorException e) {
            Platform.writeDebugLog(3, e.getLocalizedMessage());
            cache.putJsonObject(deviceCredentials.getIp(), null);
            Platform.writeDebugLog(3, Thread.currentThread().getName() +
                    " [SingleWorker] JSON object updated as NULL ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl());
        }
    }

    private void updateLigowave() {
        Platform.writeDebugLog(3, Thread.currentThread().getName() +
                " [SingleWorker] Calling update ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl() + " , deviceCredentials : "
                + deviceCredentials.hashCode());
        try {
            Ligowave ligowave = Rest.updateLigowaveJsonObject(deviceCredentials, Settings.LOGIN_TRIES_COUNT);
            cache.putJsonObject(deviceCredentials.getIp(), ligowave);

            // DEBUG OUTPUT
            if (ligowave == null)
                Platform.writeDebugLog(3, Thread.currentThread().getName() +
                        " [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl()
                        + " JSON obj : null");
            else
                Platform.writeDebugLog(3, Thread.currentThread().getName() +
                        " [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl()
                        + " JSON obj : " + ligowave.getClass().getName());
        } catch (CollectorException e) {
            Platform.writeDebugLog(3, e.getLocalizedMessage());
            cache.putJsonObject(deviceCredentials.getIp(), null);
            Platform.writeDebugLog(3, Thread.currentThread().getName() +
                    " [SingleWorker] JSON object updated as NULL ! EntryPoint :" + type.toString() + " URL : " + deviceCredentials.getUrl());
        }
    }

    public void stop() {
        this.cancel();
        this.purge();
    }
}


