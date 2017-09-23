import subprocess
from subprocess import Popen, PIPE, STDOUT
import time

cmd2_2 = 'askbirthmonth2.exe'

the_directory = "C:/MinGW/msys/1.0/home/YuzuruM"
p = Popen(cmd2_2.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)
print p.stdout.readline()
time.sleep(3)
p.stdin.write("10\n")
print p.stdout.readline()
