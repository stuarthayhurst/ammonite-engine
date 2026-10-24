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
os.close(slave)

#Extract command's output
output = []
finished = False
while True:
  try:
    data = os.read(master, 1024).decode()
  except OSError as e:
    #Quit when the process finishes and the buffer has been consumed
    break
  except KeyboardInterrupt:
    #Quit when cancelled
    exit(1)

  if not data:
    #Quit if no data remains and the process had already finished
    if finished:
      break

    #If the process finished, consume the rest of the buffer, then quit
    if process.poll():
      finished = True

  output.append(data)

output = "".join(output)

#Get the return code
finished = False
while not finished:
  try:
    process.wait(timeout = 10)
    finished = True
  except TimeoutExpired:
    print("Process didn't exit after 10 seconds", file = sys.stderr)

exitCode = process.returncode

#Print the output and exit
lockedPrint(output, "", syncFile)
exit(exitCode)
