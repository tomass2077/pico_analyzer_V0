import os
import subprocess
import shutil
from time import sleep

if (not os.path.exists("./build")):
    os.makedirs("build")


subprocess.run("cd build && cmake .. && make", shell=True)

while (True):
    if (os.path.exists("/media/tomass/RPI-RP2/")):
        sleep(.5)
        subprocess.run("cd build && cmake .. && make", shell=True)
        sleep(.25)
        if (os.path.exists("/media/tomass/RPI-RP2/")):
            file_stats = os.stat("build/full_build.uf2")
            print(file_stats.st_size/1024/1024*8, 'mbit')
            shutil.copy("build/full_build.uf2", "/media/tomass/RPI-RP2/")
        sleep(1)
    sleep(1)
    print(".")
    # break
