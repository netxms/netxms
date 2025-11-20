# Contributing to NetXMS

Thank you for your interest in contributing to NetXMS! This document provides guidelines and information about how to contribute to the project effectively.

## 📋 Table of Contents

- [Project Vision & Values](#project-vision--values)
- [Getting Started](#getting-started)
- [Development Priorities](#development-priorities)
- [Contribution Workflow](#contribution-workflow)
- [Development Setup](#development-setup)
- [Code Guidelines](#code-guidelines)
- [Testing Requirements](#testing-requirements)
- [Documentation](#documentation)
- [Community Guidelines](#community-guidelines)

## 🎯 Project Vision & Values

NetXMS is committed to providing a powerful, scalable, and privacy-respecting network monitoring solution. Our core values guide all development decisions:

### Privacy First
- **No user data transmission**: NetXMS does not transmit user data to external parties or developers
- **Local data control**: All monitoring data remains under user control within their infrastructure
- **Transparent operations**: No hidden telemetry or analytics collection

### Performance Excellence
- **Efficiency focus**: Every feature should be optimized for minimal resource usage
- **Scalability**: Solutions must work efficiently from small setups to enterprise environments
- **Real-time processing**: Prioritize low-latency data collection and processing

### Modular Architecture
- **Modular approach**: Extend functionality through database drivers, modules and device drivers
- **Subagent system**: Use modular subagents for specific monitoring capabilities
- **Plugin ecosystem**: Support extensibility without modifying core components

### Distributed Scalability
- **Proxy architecture**: Enable horizontal scaling through intelligent proxy deployment
- **Zone-based monitoring**: Support logical network segmentation and distributed data collection
- **Autonomous operation**: Maintain functionality even when central components are unavailable

## 🚀 Getting Started

### Prerequisites

Before contributing, ensure you have:
- Experience with C/C++ for server and agent development
- Java and SWT/RWT knowledge for client applications and APIs
- Understanding of network protocols (SNMP, SSH, HTTP/HTTPS) for some tasks
- Basic understanding of SQL
- Git version control experience

### Initial Steps

1. **Join the community**:
   - [Forum](https://www.netxms.org/forum/index.php) - Introduce yourself and discuss ideas
   - [Telegram](https://telegram.me/netxms) - Real-time discussions
   - [Issue Tracker](https://github.com/netxms/netxms/issues) - Review existing issues

2. **Understand the codebase**:
   - Read the [Concepts Documentation](https://www.netxms.org/documentation/adminguide/concepts.html)
   - Review the [C++ Developer Guide](./doc/NETXMS_CPP_DEVELOPER_GUIDE.md) for core development
   - Review existing code to understand patterns and conventions
   - Set up a development environment

3. **Start small**:
   - Look for issues labeled "good first issue" or "help wanted"
   - Fix documentation typos or improve examples
   - Add unit tests for existing functionality

## 🎯 Development Priorities

NetXMS development follows these priorities to maintain project coherence and quality:

### 1. Security & Privacy (Highest Priority)
- Vulnerability fixes
- Data leakage prevention


### 2. Core Stability & Performance (High Priority)
- Critical bug fixes in core monitoring engine
- Memory leak fixes and performance optimizations
- Database query optimization

### 3. Monitoring Capabilities (Medium Priority)
- New protocol support
- New network device drivers 
- Improvements to network device support
- New subagent development
- Data collection method improvements

### 4. User Interface & Experience (Medium Priority)
- Web client improvements
- Desktop client enhancements
- Visualization capabilities

### 5. Integration & Automation (Lower Priority)
- Third-party system integrations
- Automation script improvements
- API extensions
- Notification system enhancements
- Improvement of AI tools

### Guidelines for Priority Assessment
- **Performance impact**: Changes affecting system performance get higher priority
- **User base impact**: Features benefiting many users are prioritized
- **Maintenance burden**: Prefer solutions that don't increase long-term maintenance
- **Architectural alignment**: Features that fit the modular, scalable architecture are preferred

## 🔄 Contribution Workflow

NetXMS follows an **issue-first** approach to ensure all contributions align with project goals and avoid duplicate work.

### 1. Issue Creation (Required)

**Before starting any work**, create an issue that describes:
- **Problem description**: What problem does this solve?
- **Proposed solution**: How do you plan to solve it?
- **Impact assessment**: Who benefits and how?
- **Implementation approach**: Technical details of your approach

**Issue types**:
- 🐛 **Bug Report**: Use the bug report template
- ✨ **Feature Request**: Use the feature request template
- 📖 **Documentation**: For documentation improvements create issues in dedicated documentation repositories: [NXSL Documentation](https://github.com/netxms/nxsl-doc) or [NetXMS Documentation](https://github.com/netxms/netxms-doc)

### 2. Discussion & Approval

- Community discussion on the issue
- Maintainer review and feedback
- Approval before implementation begins
- Scope clarification if needed

### 3. Implementation

Only after issue approval:
- Fork the repository
- Create a feature branch: `git checkout -b feature/issue-number-description`
- Implement your changes following our guidelines
- Test your changes thoroughly (see Testing Requirements below)
- Update documentation as needed

### 4. Pull Request Submission

- Reference the issue: "Fixes #123" or "Addresses #123"
- Provide clear description of changes
- Include test results and performance impact
- **License Agreement**: By submitting a pull request, you agree to license your contributions under the MIT License
- Request review from maintainers

### 5. Review Process

- Code review by maintainers
- Manual testing on multiple platforms if applicable
- Documentation review
- Final approval and merge

## 🛠️ Development Setup

### Building from Source

1. **Clone the repository**:
   ```bash
   git clone https://github.com/netxms/netxms.git
   cd netxms
   ```

2. **Install dependencies (Ubuntu/Debian)**:
   ```bash
   sudo apt-get update
   sudo apt-get install build-essential g++ automake git gitk libtool flex bison libssl-dev libjansson-dev autoconf valgrind gdb libcurl4-openssl-dev postgresql-client maven mariadb-client libpcre3-dev mosquitto-dev libmariadb-dev libpq-dev default-jdk libasan8
   ```

3. **Configure and build**:
Configure NetXMS to be built in `/opt/netxms` with desired options:

   ```bash
   mkdir /opt/netxms
   cd ./tools
   ./updatetag.pl #run once after clone to generate version.h
   cd ..
   ./reconf #run once after clone 
   ./configure --prefix=/opt/netxms --with-sqlite --with-server --with-agent --with-tests --enable-debug --enable-sanitizer #choose --with-mysql --with-pgsql --with-oracle --with-mssql as needed
   make
   make install 
   ```

> note: --enable-sanitizer is optional, it enables address sanitizer for memory issue detection, sanitizer and valgrind usage is mutually exclusive

4. **Database setup**:
Create and configure your database (PostgreSQL, MySQL, etc.) as per [Installation Guide](https://www.netxms.org/documentation/adminguide/installation.html).

5. **Run tests**:
   ```bash
   chmod +x ./tests/suite/netxms-test-suite
   ./tests/suite/netxms-test-suite

   ```

6. **Run the server and agent**:
   ```bash
   /opt/netxms/bin/netxmsd -D6 #run server with debug level 6 not as a daemon
   /opt/netxms/bin/netxms-agent -D6 #run agent with debug level 6 not as a daemon
   ```

7. **Build and run the Java clients**:

Build base and client libraries:

   ```bash
   mvn -f  ./src/java-common/netxms-base/pom.xml install -DskipTests -Dmaven.javadoc.skip=true 
   mvn -f ./src/client/java/netxms-client/pom.xml install -DskipTests -Dmaven.javadoc.skip=true
   ```

Build desktop standalone client:

   ```bash
   mvn -f ./src/client/nxmc/java/pom.xml clean package -Pdesktop -Pstandalone
   ```

Build web client:

   ```bash
   mvn -f ./src/client/nxmc/java/pom.xml clean package -Pweb
   ```

Run the client:

   ```bash
   java -jar ./src/client/nxmc/java/target/netxms-nxmc-standalone-<version>.jar
   ```

Run the web client:

   ```bash
   mvn -f ./src/client/nxmc/java/pom.xml clean package -Pweb -Dnetxms.build.disablePlatformProfile=true
   mvn -f ./src/client/nxmc/java/pom.xml jetty:run -Pweb -Dnetxms.build.disablePlatformProfile=true
   ```

### Development Environment

- **IDE recommendations**: Eclipse for C/C++ and Java development, repository includes project files
- **Debugging**: Use gdb for core components, standard Java debugging for client
- **Static analysis**: Use tools like sanitizer or valgrind for memory analysis (sanitizer and valgrind usage is mutually exclusive)
- **Testing**: Unit tests

## 📝 Code Guidelines

### C/C++ Code Standards

Please refer to the [C++ Developer Guide](./doc/NETXMS_CPP_DEVELOPER_GUIDE.md) for detailed coding standards and best practices.

### Java Code Standards

- **Style**: Follow standard Java conventions
- **Dependencies**: Minimize external dependencies, prefer standard library
- **Exception handling**: Proper exception handling with meaningful messages
- **Documentation**: Javadoc for public APIs

### General Principles

- **Modularity**: Keep components loosely coupled.
- **Performance**: Consider performance impact of all changes
- **Compatibility**: Maintain backward compatibility when possible
- **Logging**: Use appropriate logging levels and meaningful messages
- **Configuration**: Make new features configurable when appropriate

## 🧪 Testing Requirements

### Current Testing Infrastructure

NetXMS currently has limited automated testing infrastructure:

- **Unit tests**: Available for some server base classes (String, StringBuilder, etc.)
- **Manual testing**: Primary testing method for most functionality
- **Platform testing**: Manual verification across Windows, Linux, macOS
- **Integration tests**: Limited coverage, primarily manual

### Testing Guidelines

- **Test your changes**: Always test your changes manually before submitting
- **Cross-platform verification**: Test on multiple platforms when possible
- **Regression testing**: Ensure existing functionality still works
- **Performance impact**: Verify that changes don't negatively affect performance
- **Edge cases**: Test error conditions and boundary cases
- **Documentation testing**: Verify that examples in documentation work correctly

### Manual Testing Process

Since we don't have comprehensive automated tests, contributors should:

1. **Build and test locally**: Ensure your changes compile and work as expected
2. **Test core functionality**: Verify that basic monitoring operations still work
3. **Test your specific changes**: Thoroughly exercise the new functionality
4. **Test error conditions**: Ensure proper error handling and reporting
5. **Performance testing**: Monitor resource usage for performance-sensitive changes

### Future Testing Goals

We welcome contributions to improve our testing infrastructure:
- Expanding unit test coverage for core classes
- Integration tests for key workflows
- Automated regression testing

## 📚 Documentation

Documentation should be updated in separate repositories:

* [NetXMS Documentation](https://github.com/netxms/netxms-doc)
* [NXSL Documentation](https://github.com/netxms/nxsl-doc) 
* [Data dictionary](https://github.com/netxms/netxms-db-doc) 

### Documentation Requirements

- **Code documentation**: Comment complex algorithms and public APIs
- **User documentation**: Update the [NetXMS Administration Guide](https://github.com/netxms/netxms-doc/tree/master/admin) or the [NXSL Guide](https://github.com/netxms/nxsl-doc) for new features
- **API documentation**: Maintain API documentation for changes using javadoc or OpenAPI specs
- **Change documentation**: Update [changelog](https://github.com/netxms/changelog/blob/master/ChangeLog.md) for all user-visible changes

### Documentation Standards

- Clear, concise language
- Examples for complex features
- Screenshots for UI changes
- Version compatibility information

## 🤝 Community Guidelines

### Code of Conduct

- Be respectful and professional in all interactions
- Focus on constructive feedback and collaboration
- Welcome newcomers and help them get started
- Respect different perspectives and experience levels

### Communication

- **English language**: All communication should be in English
- **Clear communication**: Be specific and provide context
- **Patience**: Allow time for review and feedback
- **Constructive criticism**: Focus on the code, not the person

### Getting Help

- **Forum**: For general discussions and questions
- **Telegram**: For real-time help and community chat
- **Issue tracker**: For bug reports and feature discussions
- **Documentation**: Check existing docs before asking questions

## 📞 Contact

- **Forum**: [https://www.netxms.org/forum/index.php](https://www.netxms.org/forum/index.php)
- **Telegram**: [https://telegram.me/netxms](https://telegram.me/netxms)
- **Issue Tracker**: [https://github.com/netxms/netxms/issues](https://github.com/netxms/netxms/issues)
- **Website**: [https://www.netxms.org](https://www.netxms.org)

---

Thank you for contributing to NetXMS! Your efforts help make network monitoring better for everyone while respecting privacy and maintaining high performance standards.