#!/usr/bin/env python3

import sys
import re
import os
import locale
import functools

JAVA_DICTIONARY_PATTERN = re.compile(r".* ([a-zA-Z0-9_\\-]+);.*")
JAVA_DICTIONARY_LINE_START = re.compile(r".*public static String .*")
locales = ['ar', 'cs', 'de', 'es', 'fr', 'pt', 'ru', 'zh_CN']

def getValue(line, isJavaFile):
    if isJavaFile:
        return JAVA_DICTIONARY_PATTERN.search(line).group(1)
    else:
        return line.rstrip()

def removeLines(fileIn, valueArray, pattern, isJavaFile):
    f = open(fileIn,"r")
    lines = f.readlines()
    f.close()
    f = open(fileIn,"w")
    for line in lines:
        if isJavaFile and not JAVA_DICTIONARY_LINE_START.match(line):
            f.write(line)
            continue
        if pattern.search(line):
            print(getValue(line, isJavaFile))
            valueArray.append(getValue(line, isJavaFile))
            #remove line
        else:
            f.write(line)
    locale.setlocale(locale.LC_ALL, 'en_US.UTF-8') 
    valueArray.sort(key=functools.cmp_to_key(lambda self, other: locale.strcoll(self.split('=')[0], other.split('=')[0])))
    f.close()

def getValueArray(fileOut, isJavaFile):
    arr = []
    f = open(fileOut,"r")
    lines = f.readlines()
    f.close()
    for line in lines:
        if isJavaFile and not JAVA_DICTIONARY_LINE_START.match(line):
            continue
        arr.append(getValue(line, isJavaFile))         
    return arr

def recreateLinesJava(fileOut, array):    
    f = open(fileOut,"r")
    lines = f.readlines()
    f.close()
    f = open(fileOut,"w")

    first = True
    for line in lines:
        if not JAVA_DICTIONARY_LINE_START.match(line):
            f.write(line)
            continue
        if first:
            first = False
            for i in array:
                f.write("   public static String {};\n".format(i))
    f.close()

def recreateLines(fileOut, array): 
    f = open(fileOut,"w")
    for i in array:
        f.write("{}\n".format(i))
    f.close()

if len(sys.argv) < 4:
    print("Please provide next parameters plugin path from, plugin path to, message regexp")
    exit(1)

pattern = re.compile(sys.argv[3])

fileIn = sys.argv[1]+ "/Messages.java"
fileOut = sys.argv[2]+ "/Messages.java"

print("Names moved in Messages.java file:")
valueArray = getValueArray(fileOut, True)
removeLines(fileIn, valueArray, pattern, True)
recreateLinesJava(fileOut, valueArray)

fileIn = sys.argv[1]+ "/messages.properties"
fileOut = sys.argv[2]+ "/messages.properties"

print("Names moved in messages.properties file:")
valueArray = getValueArray(fileOut, False)
removeLines(fileIn, valueArray, pattern, False)
recreateLines(fileOut, valueArray)

for lc in locales:
    fileIn = "{}/messages_{}.properties".format(sys.argv[1], lc)
    fileOut = "{}/messages_{}.properties".format(sys.argv[2], lc)

    print("Names moved in messages_{}.properties file:".format(lc))
    valueArray = getValueArray(fileOut, False)
    removeLines(fileIn, valueArray, pattern, False)
    recreateLines(fileOut, valueArray)

#open Messages.java in
#find line, read
#open messages out
#find where to put put line, remove from in file
#Do the same action for all .properties files 