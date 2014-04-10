package com.radensolutions.reporting.infrastructure.impl;

import java.io.IOException;
import java.util.List;

import name.pachler.nio.file.ClosedWatchServiceException;
import name.pachler.nio.file.FileSystems;
import name.pachler.nio.file.Path;
import name.pachler.nio.file.Paths;
import name.pachler.nio.file.StandardWatchEventKind;
import name.pachler.nio.file.WatchEvent;
import name.pachler.nio.file.WatchKey;
import name.pachler.nio.file.WatchService;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Component;

import com.radensolutions.reporting.infrastructure.AutoDeployer;

@Component
public class FileSystemDeployer implements AutoDeployer
{

	private static final Logger logger = LoggerFactory.getLogger(FileSystemDeployer.class);
	private String scanDirectory;
	private String workspaceDirectory;
	private WatchService watchService;

	private void deploy(String fileName)
	{
		System.out.println("### deploy: " + fileName);
	}

	private void undeploy(String fileName)
	{
		System.out.println("### undeploy: " + fileName);
	}

	private void redeploy(String fileName)
	{
		System.out.println("### redeploy: " + fileName);
	}

	@Override
	public void start()
	{
		watchService = FileSystems.getDefault().newWatchService();
		final Path path = Paths.get(scanDirectory);
		try
		{
			path.register(watchService, StandardWatchEventKind.ENTRY_CREATE, StandardWatchEventKind.ENTRY_DELETE, StandardWatchEventKind.ENTRY_MODIFY);

			final Thread thread = new Thread(new Runnable()
			{
				@Override
				public void run()
				{
					while (true)
					{
						WatchKey signalledKey;
						try
						{
							signalledKey = watchService.take();
						} catch (InterruptedException e)
						{
							continue;
						} catch (ClosedWatchServiceException e)
						{
							break;
						}

						List<WatchEvent<?>> list = signalledKey.pollEvents();

						signalledKey.reset();

						for (WatchEvent<?> e : list)
						{
							System.out.println(e);
							if (e.kind() == StandardWatchEventKind.ENTRY_CREATE)
							{
								Path context = (Path) e.context();
								deploy(context.toString());
							}
							else if (e.kind() == StandardWatchEventKind.ENTRY_DELETE)
							{
								Path context = (Path) e.context();
								undeploy(context.toString());
							}
							else if (e.kind() == StandardWatchEventKind.ENTRY_MODIFY)
							{
								Path context = (Path) e.context();
								redeploy(context.toString());
							}
						}
					}
				}
			});
			thread.start();
		} catch (UnsupportedOperationException e)
		{
			logger.error("file watching not supported!", e);
		} catch (IOException e)
		{
			logger.error("I/O error", e);
		}
	}

	@Override
	public void stop()
	{
		try
		{
			if (watchService != null)
			{
				watchService.close();
			}
		} catch (IOException e)
		{
			logger.error("I/O error", e);
		}
	}

	public String getScanDirectory()
	{
		return scanDirectory;
	}

	public void setScanDirectory(String scanDirectory)
	{
		this.scanDirectory = scanDirectory;
	}

	public String getWorkspaceDirectory()
	{
		return workspaceDirectory;
	}

	public void setWorkspaceDirectory(String workspaceDirectory)
	{
		this.workspaceDirectory = workspaceDirectory;
	}

	@Override
	public String toString()
	{
		return "FileSystemDeployer{" + "workspaceDirectory='" + workspaceDirectory + '\'' + ", scanDirectory='" + scanDirectory + '\'' + '}';
	}
}
