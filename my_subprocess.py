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
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n")
print "1"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("2000\n")
print "2000"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("Toyota\n")
print "Toyota"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("Crown\n")
print "Crown"   #Q05

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q06
print "1"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q07
print "1"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q08
print "1"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q09
print "1 Conventional"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q10
print "1 US OBD-II"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q11 
print "1 Light/Medium"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n") #Q13
print "1 ??"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n")
print "1"

for num in range(1,20):
    current_sentense = p.stdout.readline()
    if_enter = ("Enter" in current_sentense)or("How many" in current_sentense)
    print current_sentense 
    if if_enter:
            break
p.stdin.write("1\n")
print "1"




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
