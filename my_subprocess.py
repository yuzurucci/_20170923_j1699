import subprocess
from subprocess import Popen, PIPE, STDOUT
import time

cmd = 'j1699.exe'

#the_directory = "C:/MinGW/msys/1.0/home/YuzuruM"
the_directory = "C:/Users/YuzuruM/Documents/MATLAB/-20170222_sampleIOModelwithCAN_forDT_20170923_j1699"
p = Popen(cmd.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)
print p.stdout.readline()
time.sleep(3)
p.stdin.write("1\n")
print p.stdout.readline()
