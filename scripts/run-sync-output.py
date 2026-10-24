#!/usr/bin/python3
import os, sys
import fcntl
import pty
import subprocess

#Run a command and synchronously print its output
#Uses pty to mimic a terminal for full colour output
#Usage: ./run-sync-output.py [sync file] [command]

#Disclaimer: This isn't general purpose, and probably won't work for another project
#This specifically works with the behaviour of this project's Makefile

def lockedPrint(output, ending, lockFile):
  fcntl.flock(lockFile, fcntl.LOCK_EX)
  print(output, end = ending)
  fcntl.flock(lockFile, fcntl.LOCK_UN)

#Process inputs
syncFilePath = sys.argv[1]
command = sys.argv[2:]
commandString = " ".join(command)

#Open the lock file, create if missing
syncFile = None
try:
  syncFile = open(syncFilePath, "x+")
except FileExistsError:
  syncFile = open(syncFilePath, "r+")

#Print the command to be run
lockedPrint(commandString, "\n", syncFile)

#Run the command in a psuedo-terminal
master, slave = pty.openpty()
process = subprocess.Popen(commandString, shell=True, stdout=slave,
                           stdin=slave, stderr=slave, close_fds=True)
try:
  process.wait()
except KeyboardInterrupt:
  exit(1)
exitCode = process.returncode
os.close(slave)

#Extract command's output
output = []
while True:
  try:
    data = os.read(master, 1024).decode()
  except OSError:
    break
  except KeyboardInterrupt:
    break
  if not data:
    break
  output.append(data)
output = "".join(output)

#Print the output and exit
lockedPrint(output, "", syncFile)
exit(exitCode)
