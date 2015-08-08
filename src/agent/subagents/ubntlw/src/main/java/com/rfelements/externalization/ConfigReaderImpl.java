package com.rfelements.externalization;

import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.Access;
import org.netxms.agent.Config;
import org.netxms.agent.SubAgent;
/**
 * @author Pichanič Ján
 */
public class ConfigReaderImpl implements ConfigReader {

	private static final String DATACOLLECTOR_CONFIG_BASIC_PATH = "/datacollector/";

	private static ConfigReaderImpl instance;

	private Cache cache;

	private Config config;

	private ConfigReaderImpl() {
		SubAgent.writeLog(SubAgent.LogLevel.INFO, "[" + this.getClass().getName() + "] Config reader initialized ...");
		this.cache = CacheImpl.getInstance();
	}

	public static ConfigReaderImpl getInstance() {
		if (instance == null)
			instance = new ConfigReaderImpl();
		return instance;
	}

	@Override
	public void setConfig(Config config) {
		this.config = config;
	}

	@Override
	public void readExternalizatedVariables() {
		if (config == null) {
			SubAgent.writeLog(SubAgent.LogLevel.ERROR, "[" + this.getClass().getName() + "] [readExternalizatedVariables] Config is null !");
			return;
		}

		SubAgent.writeLog(SubAgent.LogLevel.INFO, "[" + this.getClass().getName() + "] [readExternalizatedVariables] Variables initialization ... ");
		ExternalConstants.CONNECTION_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "CONNECTION_TIMEOUT", 8000);
		ExternalConstants.REQUEST_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "REQUEST_TIMEOUT", 8000);
		ExternalConstants.SOCKET_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "SOCKET_TIMEOUT", 8000);
		ExternalConstants.LOGIN_TRIES_COUNT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "LOGIN_TRIES_COUNT", 3);
		ExternalConstants.UPDATE_PERIOD = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "UPDATE_PERIOD", 20000);

		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				"[" + this.getClass().getName() + "] [readExternalizatedVariables] CONNECTION_TIMEOUT  : " + ExternalConstants.CONNECTION_TIMEOUT);
		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				"[" + this.getClass().getName() + "] [readExternalizatedVariables] REQUEST_TIMEOUT     : " + ExternalConstants.REQUEST_TIMEOUT);
		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				"[" + this.getClass().getName() + "] [readExternalizatedVariables] SOCKET_TIMEOUT      : " + ExternalConstants.SOCKET_TIMEOUT);
		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				"[" + this.getClass().getName() + "] [readExternalizatedVariables] LOGIN_TRIES_COUNT   : " + ExternalConstants.LOGIN_TRIES_COUNT);
		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				"[" + this.getClass().getName() + "] [readExternalizatedVariables] UPDATE_PERIOD       : " + ExternalConstants.UPDATE_PERIOD);

	}

	@Override
	public synchronized Access getAccess(String protocol, String basicPath, String ip) throws CollectorException {
		if (config == null) {
			SubAgent.writeLog(SubAgent.LogLevel.ERROR, "[" + this.getClass().getName() + "] [getAccess] Config is NULL");
			throw new CollectorException("Config is NULL");
		}
		String key = protocol + ip + basicPath;
		if (cache.containsAccess(key)) {
			return cache.getAccess(key);
		}

		String value = config.getValue(basicPath + ip, "");
		if (value.equals("")) {
			SubAgent.writeLog(SubAgent.LogLevel.ERROR, " [" + this.getClass().getName() + "] [getAccess] Device IP not found in config file !");
			throw new CollectorException("Device IP not found in config file !");
		}

		SubAgent.writeLog(SubAgent.LogLevel.INFO,
				" " + Thread.currentThread().getName() + " [" + this.getClass().getName() + "] [getAccess] Accessing entry point " + protocol + ip
						+ basicPath + " , result : " + value + "");
		String[] split = value.split(":");
		Access access = new Access(protocol, ip, split[0], split[1]);
		cache.putAccess(key, access);
		return access;
	}
}
