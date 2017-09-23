import subprocess
from subprocess import Popen, PIPE, STDOUT
import time

cmd = 'j1699.exe'
cmd2 = 'askbirthmonth2.exe'
#the_directory = "C:/MinGW/msys/1.0/home/YuzuruM"
the_directory = "C:/Users/YuzuruM/Documents/MATLAB/_20170923_j1699/j1699_visualstudio"

p = Popen(cmd.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)
#p = Popen(cmd2.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)
while True:
    line = p.stdout.readline()
    print p.stdout.readline()
    if not line:
        break
#time.sleep(3)
#p.stdin.write("1\n")
#print p.stdout.readline()
