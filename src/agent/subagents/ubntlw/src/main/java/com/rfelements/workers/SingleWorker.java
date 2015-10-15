package com.rfelements.workers;
import com.rfelements.DeviceType;
import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.exception.CollectorException;
import com.rfelements.externalization.ExternalConstants;
import com.rfelements.model.Access;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.rest.Rest;
import org.netxms.agent.SubAgent;

import java.util.Timer;
import java.util.TimerTask;
/**
 * @author Pichanič Ján
 */
public class SingleWorker extends Timer {

	private final Access access;

	private final DeviceType type;

	private Cache cache = CacheImpl.getInstance();

	public SingleWorker(Access access, DeviceType type) {
		super(true);
		this.access = access;
		this.type = type;
		SubAgent.writeDebugLog(3, Thread.currentThread().getName() + " [SingleWorker] Construction ...");
		SubAgent.writeDebugLog(3,
				Thread.currentThread().getName() + " [SingleWorker] Args - access : " + access.hashCode() + ", type : " + type.toString());
	}

	public void start() {
		this.scheduleAtFixedRate(new TimerTask() {

			@Override
			public void run() {
				SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
						" [SingleWorker] Reschedule, EntryPoint : " + type.toString() + "  URL : " + access.getProtocol() + access.getIp()
						+ " , access : " + access.hashCode());
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
		}, 0, ExternalConstants.UPDATE_PERIOD);
	}

	private void updateUbiquiti() {
		SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
				" [SingleWorker] Calling update ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp() + " , access : "
				+ access.hashCode());
		try {
			Ubiquiti ubnt = Rest.updateUbiquitiJsonObject(access, ExternalConstants.LOGIN_TRIES_COUNT);
			cache.putJsonObject(access.getIp(), ubnt);

			// DEBUG OUTPUT
			if (ubnt == null)
				SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
						" [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp()
						+ " JSON obj : null");
			else
				SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
						" [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp()
						+ " JSON obj : " + ubnt.getClass().getName());
		} catch (CollectorException e) {
			SubAgent.writeDebugLog(3, e.getLocalizedMessage());
			cache.putJsonObject(access.getIp(), null);
			SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
					" [SingleWorker] JSON object updated as NULL ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access
					.getIp());
		}
	}

	private void updateLigowave() {
		SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
				" [SingleWorker] Calling update ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp() + " , access : "
				+ access.hashCode());
		try {
			Ligowave ligowave = Rest.updateLigowaveJsonObject(access, ExternalConstants.LOGIN_TRIES_COUNT);
			cache.putJsonObject(access.getIp(), ligowave);

			// DEBUG OUTPUT
			if (ligowave == null)
				SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
						" [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp()
						+ " JSON obj : null");
			else
				SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
						" [SingleWorker] JSON object updated ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access.getIp()
						+ " JSON obj : " + ligowave.getClass().getName());
		} catch (CollectorException e) {
			SubAgent.writeDebugLog(3, e.getLocalizedMessage());
			cache.putJsonObject(access.getIp(), null);
			SubAgent.writeDebugLog(3, Thread.currentThread().getName() +
					" [SingleWorker] JSON object updated as NULL ! EntryPoint :" + type.toString() + " URL : " + access.getProtocol() + access
					.getIp());
		}
	}

	public void stop() {
		this.cancel();
		this.purge();
	}
}


