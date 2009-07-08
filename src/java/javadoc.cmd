@echo off

mvn -Dmaven.test.skip=true javadoc:javadoc %*
