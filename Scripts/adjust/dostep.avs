LoadPlugin("../Avisynth/plugins64/GetSystemEnv.dll")
LoadPlugin("../Avisynth/plugins64/autoadjust.dll")
LoadPlugin("../Avisynth/plugins64/AutoLevels_x64.dll")
LoadPlugin("../Avisynth/plugins64/warpsharp.dll")
LoadPlugin("../Avisynth/plugins64/addgrainc.dll")
film = GetSystemEnv("input")
setMemoryMax(8192)
source= AviSource(film,audio=false)

sharpened=source.unsharpmask(30,3,0).blur(0.8).unsharpmask(50,2,0).blur(0.8)
adjusted=sharpened.tweak(sat=0.8).autoadjust(auto_gain=true,bright_limit=1,auto_balance=true,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=1)
#grained=adjusted.addgrain(20,0,0)

#Eval("grained")
Eval("adjusted")


Prefetch(8)
