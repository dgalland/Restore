# make file for Deflicker.dll (by Fizick)
# Use these for MS VC6 or 7 
CXX=cl.exe
CXXFLAGS= /LD /Ox /O2 /D "NDEBUG" /G7

LINK32=link.exe
LINK32FLAGS= /dll /nologo

# Resource compiler. 
RC=rc.exe

deflicker.dll: deflicker.obj info.obj deflicker.res
	$(LINK32) $(LINK32FLAGS) deflicker.obj info.obj deflicker.res /out:deflicker.dll

deflicker.obj: deflicker.cpp
	$(CXX) $(CXXFLAGS) deflicker.cpp -c

info.obj: info.cpp
	$(CXX) $(CXXFLAGS) info.cpp -c

deflicker.res: deflicker.rc
	$(RC) deflicker.rc
