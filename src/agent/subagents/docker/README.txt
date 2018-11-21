Build:

	mvn clean compile assembly:single 

Agent configuration:

	Subagent=java.nsm

	[java]
	ClassPath=/path/to/docker-1.0-SNAPSHOT-jar-with-dependencies.jar
	Plugin=org.netxms.subagent.docker.DockerPlugin
