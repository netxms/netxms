package com.github.tomaskir.netxms.subagents.bind9;

import com.github.tomaskir.netxms.subagents.bind9.collection.CollectionResult;
import com.github.tomaskir.netxms.subagents.bind9.collection.Collector;
import com.github.tomaskir.netxms.subagents.bind9.exceptions.CollectionErrorException;
import org.netxms.agent.Config;
import org.netxms.agent.Parameter;
import org.netxms.agent.ParameterType;
import org.netxms.agent.Plugin;
import org.netxms.agent.adapters.ParameterAdapter;

import java.nio.file.Paths;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.locks.ReadWriteLock;
import java.util.concurrent.locks.ReentrantReadWriteLock;

/**
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class Bind9Plugin extends Plugin {

    private static final String NAME = "bind9 Java Subagent Plugin";
    private static final String VERSION = "2.1-M1";

    private final Parameters supportedParameters = new Parameters();
    private final ReadWriteLock dataCollectionLock = new ReentrantReadWriteLock();
    private final CollectionResult collectionResult = new CollectionResult();

    private final Collector collector;
    private final Thread collectionThread;

    // constructor
    public Bind9Plugin(Config config) {
        super(config);

        // get config values
        String statsFile = config.getValue("/bind9/StatisticsFile", "");
        long collectionInterval = config.getValueLong("/bind9/CollectionInterval", -1);

        // check if all config values present
        if (statsFile.equals("")) {
            throw new IllegalStateException("'statsFile' configuration value not found " +
                    "in 'bind9' section of Agent configuration");
        }
        if (collectionInterval < 1) {
            throw new IllegalStateException("'collectionInterval' configuration value not found or lower than 1" +
                    "in 'bind9' section of Agent configuration");
        }

        this.collector = new Collector(
                collectionInterval,
                Paths.get(statsFile),
                supportedParameters,
                collectionResult,
                dataCollectionLock
        );

        this.collectionThread = new Thread(collector);
    }

    @Override
    public void init(Config config) {
        super.init(config);

        // start the collection thread
        collectionThread.start();
    }

    @Override
    public void shutdown() {
        super.shutdown();

        // signal the collection thread to terminate
        collector.terminate();

        try {
            collectionThread.join();
        } catch (InterruptedException ignored) {
        }
    }

    @Override
    public Parameter[] getParameters() {
        Set<Parameter> parameterSet = new HashSet<>();

        // create parameters for all supported attributes
        for (String[] entry : supportedParameters.getList()) {
            final String parameterName = entry[0];
            final String parameterDescription = entry[1];

            parameterSet.add(
                    new ParameterAdapter(parameterName, parameterDescription, ParameterType.UINT64) {
                        @Override
                        public String getValue(String s) throws Exception {
                            // lock data collection lock for reading
                            dataCollectionLock.readLock().lock();

                            // if collection error, throw so Agent interprets it as a collection error
                            if (collectionResult.isCollectionError())
                                throw new CollectionErrorException();

                            String result = collectionResult.getResult().get(parameterName);

                            // unlock data collection lock
                            dataCollectionLock.readLock().unlock();

                            // if result not present in the result map, its value is 0 (bind9 stats behave this way)
                            if (result == null)
                                result = "0";

                            return result;
                        }
                    }
            );
        }

        // return the parameters as an array
        return parameterSet.toArray(new Parameter[parameterSet.size()]);
    }

    @Override
    public String getName() {
        return NAME;
    }

    @Override
    public String getVersion() {
        return VERSION;
    }

}
