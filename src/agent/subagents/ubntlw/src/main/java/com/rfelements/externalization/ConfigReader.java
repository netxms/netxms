package com.rfelements.externalization;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.Access;
import org.netxms.agent.Config;
/**
 * @author Pichanič Ján
 */
public interface ConfigReader {

	void setConfig(Config config);

	void readExternalizatedVariables();

	Access getAccess(String protocol, String basicPath, String ip) throws CollectorException;
}
