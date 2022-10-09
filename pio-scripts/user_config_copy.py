Import('env')
import os
import shutil

# copy ISTAR00/my_config_sample.h to ISTAR00/my_config.h
if os.path.isfile("istar00/my_config.h"):
    print ("*** use existing my_config.h ***")
else: 
    shutil.copy("istar00/my_config_sample.h", "istar00/my_config.h")
