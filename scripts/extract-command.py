#!/usr/bin/python3
import sys, os, shutil
import fcntl
import json

#Disclaimer: This isn't general purpose, and probably won't work for another project
#This specifically works with the behaviour of this project's Makefile

#Determine the project root, output file path, the compiler and source file
scriptPath = os.path.dirname(os.path.realpath(__file__))
outputPath = f"{sys.argv[1]}/compile_commands.json"
compiler = shutil.which(sys.argv[2])
inputFile = sys.argv[3]

#Determine the compiler output file and arguments used to generate it
compilerOutputFile = ""
args = [compiler]
for i in range(3, len(sys.argv)):
  if sys.argv[i] == "-o":
    compilerOutputFile = sys.argv[i + 1]
  args.append(sys.argv[i])

outputObject = {
  'directory': scriptPath,
  'file': inputFile,
  'output': compilerOutputFile,
  'arguments': args
}

#Open the compile command file, create if missing and lock it
compileCommands = None
try:
  compileCommands = open(outputPath, 'x+')
except FileExistsError:
  compileCommands = open(outputPath, 'r+')
fcntl.flock(compileCommands, fcntl.LOCK_EX)

#Read the current contents as JSON
content = compileCommands.read()
compileCommands.seek(0)
contentObject = []
if content != "":
  contentObject = json.loads(content)

#Update the loaded JSON with the new entry
written = False
for i in range(len(contentObject)):
  if contentObject[i]["file"] == inputFile:
    contentObject[i] = outputObject
    written = True
    break
if not written:
  contentObject.append(outputObject)

#Save the new JSON and release the lock
compileCommands.truncate(0)
compileCommands.write(json.dumps(contentObject))
compileCommands.flush()
fcntl.flock(compileCommands, fcntl.LOCK_UN)
compileCommands.close()
