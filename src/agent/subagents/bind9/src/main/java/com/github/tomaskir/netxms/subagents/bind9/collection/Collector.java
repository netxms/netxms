package com.github.tomaskir.netxms.subagents.bind9.collection;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.locks.ReadWriteLock;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import com.github.tomaskir.netxms.subagents.bind9.Parameters;
import com.github.tomaskir.netxms.subagents.bind9.exceptions.StatsFileRemovalException;

/**
 * @author Tomas Kirnak
 * @since 2.1-M1
 */
public final class Collector implements Runnable {

    private final long collectionInterval;
    private final Path statsFile;
    private final Parameters supportedParameters;
    private final CollectionResult result;
    private final ReadWriteLock dataCollectionLock;

    private long lastCollection = 0;
    private boolean threadRunning = true;

    // constructor
    public Collector(long collectionInterval, Path statsFile, Parameters supportedParameters,
                     CollectionResult result, ReadWriteLock dataCollectionLock) {
        this.collectionInterval = collectionInterval;
        this.statsFile = statsFile;
        this.supportedParameters = supportedParameters;
        this.result = result;
        this.dataCollectionLock = dataCollectionLock;
    }

    public void terminate() {
        threadRunning = false;
    }

    @Override
    public void run() {
        long now;
        Process rndc;
        List<String> lines;

        while (threadRunning) {
            now = System.currentTimeMillis() / 1000;

            if (now > lastCollection + collectionInterval) {
                // remove existing statistics file
                try {
                    statsFileCleanup();
                } catch (StatsFileRemovalException e) {
                    Platform.writeLog(LogLevel.WARNING,
                            "Failed to delete bind9 statistics file, error: '" + e.getCause().getMessage() + "'");

                    result.setCollectionError(true);
                    updateLastCollectionTime();
                    continue;
                }

                // run "rndc stats" to generate new statistics file
                try {
                    rndc = Runtime.getRuntime().exec("rndc stats");
                } catch (IOException e) {
                    Platform.writeLog(LogLevel.ERROR,
                            "Failed to run 'rndc stats', error: '" + e.getMessage() + "'");

                    result.setCollectionError(true);
                    updateLastCollectionTime();
                    continue;
                }

                // wait for "rndc stats" to finish
                try {
                    rndc.waitFor();
                } catch (InterruptedException ignored) {
                    rndc.destroy();
                    continue;
                }

                // check for errors
                if (rndc.exitValue() != 0) {
                    Platform.writeLog(LogLevel.ERROR,
                            "'rndc stats' exited with a non-0 exit code value");

                    result.setCollectionError(true);
                    updateLastCollectionTime();
                    continue;
                }

                // load stats from the stats file
                try {
                    lines = Files.readAllLines(statsFile, StandardCharsets.UTF_8);
                } catch (IOException e) {
                    Platform.writeLog(LogLevel.WARNING,
                            "Unable to read bind9 statistics file, error: '" + e.getMessage() + "'");

                    result.setCollectionError(true);
                    updateLastCollectionTime();
                    continue;
                }

                // lock data collection lock for writing
                dataCollectionLock.writeLock().lock();

                // remove previous values in result
                result.getResult().clear();

                // for every parameter supported by this plugin
                for (String[] entry : supportedParameters.getList()) {
                    // search all lines
                    for (String line : lines) {
                        int index = line.indexOf(entry[2]);

                        // if attribute found
                        if (index != -1) {
                            // get value of the attribute
                            String value = line.substring(0, index).trim();

                            // insert into the result map
                            result.getResult().put(entry[0], value);

                            break;
                        }
                    }
                }

                result.setCollectionError(false);
                updateLastCollectionTime();

                // unlock data collection lock
                dataCollectionLock.writeLock().unlock();
            }

            try {
                Thread.sleep(100);
            } catch (InterruptedException ignored) {
            }
        }
    }

    private void statsFileCleanup() throws StatsFileRemovalException {
        try {
            Files.delete(statsFile);
        } catch (NoSuchFileException ignored) {
            // if the file doesn't exist, we do nothing
        } catch (IOException e) {
            throw new StatsFileRemovalException(e);
        }
    }

    private void updateLastCollectionTime() {
        lastCollection = System.currentTimeMillis() / 1000;
    }

}
