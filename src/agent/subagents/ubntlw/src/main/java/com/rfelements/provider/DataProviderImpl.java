package com.rfelements.provider;
import com.rfelements.DeviceType;
import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.exception.CollectorException;
import com.rfelements.externalization.ConfigReader;
import com.rfelements.externalization.ConfigReaderImpl;
import com.rfelements.model.Access;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.workers.WorkersProvider;
import com.rfelements.workers.WorkersProviderImpl;
import org.netxms.agent.Config;
import org.netxms.agent.SubAgent;
/**
 * @author Pichanič Ján
 */
public class DataProviderImpl implements DataProvider {

	private static DataProviderImpl instance;
	private static final int DEBUG_LEVEL = 6;

	public static DataProviderImpl getInstance() {
		if (instance == null)
			instance = new DataProviderImpl();
		return instance;
	}

	private Cache cache;

	private Config config;

	private ConfigReader configReader;

	private WorkersProvider workersProvider;

	private DataProviderImpl() {
		SubAgent.writeLog(SubAgent.LogLevel.INFO, Thread.currentThread().getName() + " [DataProviderImpl] Data provider initialized ...");
		this.cache = CacheImpl.getInstance();
		this.workersProvider = WorkersProviderImpl.getInstance();
		this.configReader = ConfigReaderImpl.getInstance();
	}

	@Override
	public void setConfig(Config config) {
		// I don't know how many Entry points calling this method (I think all of them) , therefore i check if config was already instantiated
		if (this.config == null) {
			this.config = config;
			this.configReader.setConfig(config);
			this.configReader.readExternalizatedVariables();
		}
	}

	@Override
	public Ligowave getLigowaveObject(String protocol, String basicPath, String ip, DeviceType type) throws CollectorException {
		Access access = configReader.getAccess(protocol, basicPath, ip);
		this.workersProvider.startNewWorker(access, type);
		Ligowave ligowave = (Ligowave) cache.getJsonObject(access.getIp());
		if (ligowave == null) {
			String message = "Value of ligowave JSON object in the cache, under key : " + ip + " , is NULL !";
			SubAgent.writeDebugLog(DEBUG_LEVEL, message);
			throw new CollectorException(message);
		}
		return ligowave;
	}

	@Override
	public Ubiquiti getUbiquitiObject(String protocol, String basicPath, String ip, DeviceType type) throws CollectorException {
		Access access = configReader.getAccess(protocol, basicPath, ip);
		this.workersProvider.startNewWorker(access, type);
		Ubiquiti ubnt = (Ubiquiti) cache.getJsonObject(access.getIp());
		if (ubnt == null) {
			String message = "Value of ubiquiti JSON object in the cache, under key : " + ip + " , is NULL !";
			SubAgent.writeDebugLog(DEBUG_LEVEL, message);
			throw new CollectorException(message);
		}
		return ubnt;
	}

	@Override
	public void onShutdown(DeviceType type) {
		this.workersProvider.stopDeviceTypeWorkers(type);
	}
}
