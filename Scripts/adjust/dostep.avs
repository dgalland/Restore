LoadPlugin("../Avisynth/plugins64/GetSystemEnv.dll")
LoadPlugin("../Avisynth/plugins64/autoadjust.dll")
LoadPlugin("../Avisynth/plugins64/aWarpsharpMT.dll")
LoadPlugin("../Avisynth/plugins64/warpsharp.dll")
LoadPlugin("../Avisynth/plugins64/AutoLevels_x64.dll")
LoadPlugin("../Avisynth/plugins64/SmoothAdjust.dll")
LoadPlugin("../Avisynth/plugins64/GamMac_x64.dll")
film = GetSystemEnv("input")
setMemoryMax(8192)
source= AviSource(film,audio=false)

#sharpened_1=source.aWarpSharp2(type=1, thresh=100)
#Videofred recipe gives good result
sharpened_2=source.unsharpmask(30,5,0).blur(0.8).unsharpmask(50,3,0).blur(0.8).sharpen(0.1)


#Normal auto gain and white balance
adjusted_1=sharpened_2.autoadjust(auto_gain=true,auto_balance=true,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=0)

#Gamma 1.2 adjust
#adjusted_2=sharpened_2.levels(0,1.2,255,0,255).autoadjust(auto_gain=true,auto_balance=true,balance_str=1,bright_limit=1.2,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=1)

#Color correction 
#hue1 -yellow<->+blue hue2 -green<->+red
#adjusted_3=sharpened_2.levels(0,1.2,255,0,255).smoothtweak(saturation=0.8,hue1=+5, hue2=-5,tvrange=false).autoadjust(auto_gain=true,auto_balance=true,balance_str=1,bright_limit=1.2,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=0)
#adjusted_4=sharpened_2.smoothtweak(hue1=-30,hue2=-35,tvrange=false).autoadjust(auto_gain=true,auto_balance=true,balance_str=1,bright_limit=1.2,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=1)
#adjusted_5=sharpened_2.smoothtweak(hue2=-20,tvrange=false).autoadjust(auto_gain=true,auto_balance=true,balance_str=1,bright_limit=1.2,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=1)
adjusted_8=sharpened_2.smoothtweak(hue1=-20,hue2=-35,tvrange=false).smoothtweak(saturation=1.2).autoadjust(auto_gain=true,auto_balance=true,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=1)

#Gammac Useful for strong color cast
#adjusted_4=sharpened_2.convertToRgb().GamMac(show=false).convertToYV12().autoadjust(auto_gain=true,auto_balance=true,temporal_radius=3,input_tv=false,output_tv=false,gain_mode=0)

Eval("adjusted_1")


Prefetch(8)
