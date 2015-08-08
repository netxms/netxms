package com.rfelements.workers;
import com.rfelements.DeviceType;
import com.rfelements.model.Access;
import org.netxms.agent.SubAgent;

import java.util.HashMap;
/**
 * @author Pichanič Ján
 */
public class WorkersProviderImpl implements WorkersProvider {

	private static final int DEBUG_LEVEL = 7;

	public static WorkersProvider getInstance() {
		if (instance == null)
			instance = new WorkersProviderImpl();
		return instance;
	}

	private static WorkersProvider instance;

	private HashMap<DeviceType, HashMap<String, SingleWorker>> workers;

	private WorkersProviderImpl() {
		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				Thread.currentThread().getName() + " [" + this.getClass().getName() + "] Workers provider initialized !");
		this.workers = new HashMap<>();
	}

	@Override
	public void startNewWorker(Access access, DeviceType type) {
		if (!workers.containsKey(type)) {
			SingleWorker worker = new SingleWorker(access, type);
			SubAgent.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
					" [" + this.getClass().getName() + "] Single worker created ! EntryPoint : " + type.toString() + "  URL : " + access.getProtocol()
					+ access.getIp());
			HashMap<String, SingleWorker> hashMap = new HashMap<>();
			hashMap.put(access.getIp(), worker);
			workers.put(type, hashMap);
			worker.start();
		} else {
			HashMap<String, SingleWorker> list = workers.get(type);
			if (!list.containsKey(access.getIp())) {
				SingleWorker worker = new SingleWorker(access, type);
				SubAgent.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
						" [" + this.getClass().getName() + "] Single worker created ! EntryPoint : " + type.toString() + "  URL : " + access
						.getProtocol() + access.getIp());
				list.put(access.getIp(), worker);
				worker.start();
			}
		}
	}

	@Override
	public void stopDeviceTypeWorkers(DeviceType type) {
		HashMap<String, SingleWorker> hashMap = workers.get(type);
		for (String worker : hashMap.keySet()) {
			hashMap.get(worker).stop();
		}
	}
}
