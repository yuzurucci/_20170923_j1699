import subprocess
from subprocess import Popen, PIPE, STDOUT
import time

cmd = 'j1699.exe'
cmd2 = 'askbirthmonth2.exe'
#the_directory = "C:/MinGW/msys/1.0/home/YuzuruM"
the_directory = "C:/Users/YuzuruM/Documents/MATLAB/_20170923_j1699/j1699_visualstudio"

p = Popen(cmd.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)
#p = Popen(cmd2.split(), shell=True, stdin=PIPE, cwd = the_directory,stdout=PIPE)


for num in range(1,45):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)
    print current_sentense + "one sentense finished. " + str(len(current_sentense)) + " " + str(if_enter)
"""
print "found Enter choise."
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()

print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
print p.stdout.readline()
"""
"""
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
if p.stdout.readline()!="Enter choice (1 or 2): ":
    print p.stdout.readline()
else:
    print "found Enter choise."
"""
"""
for num in range(1,45):
    if p.stdout.readline()!="Enter choice (1 or 2): ":
        print p.stdout.readline()
    else:
        print "found Enter choise."
"""

"""
while True:
    line = p.stdout.readline()
    print p.stdout.readline()
    if not line:
        break
"""
#time.sleep(3)
#p.stdin.write("1\n")
#print p.stdout.readline()
