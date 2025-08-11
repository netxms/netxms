# Management Console development setup

## Prerequisites

- Java Development Kit (JDK) 17 or higher
- Apache Maven
- Eclipse IDE for Java Developers

## Setup

```sh
cd src/java-common/netxms-base/
mvn install eclipse:eclipse
cd -
cd src/client/java/netxms-client
mvn install eclipse:eclipse
cd -
cd src/client/nxmc/java
mvn eclipse:eclipse -Pdesktop
```

Then import projects to Eclipse workspace:

- src/java-common/netxms-base
- src/client/java/netxms-client
- src/client/nxmc/java

## Running

Right click on the nxmc project and select "Run As" -> "Java Application".

Search for "Startup", in package org.netxms.nxmc.
