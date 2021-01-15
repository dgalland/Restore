LoadPlugin("../Avisynth/plugins64/mvtools2.dll")
LoadPlugin("../Avisynth/plugins64/masktools2.dll")
LoadPlugin("../Avisynth/plugins64/rgtools.dll")
LoadPlugin("../Avisynth/plugins64/removedirt.dll")
LoadPlugin("../Avisynth/plugins64/fft3Dfilter.dll")
LoadPlugin("../Avisynth/plugins64/hqdn3d.dll")
LoadPlugin("../Avisynth/plugins64/awarpsharp.dll")
Import("RemoveDirt.avs")
Import("../Avisynth/plugins64/TemporalDegrain.avs")

setMemorymax(8192)
film = "E:\Test\deshake\01.avi"
source= AviSource(film,audio=false).assumefps(16).converttoYV12()
cleaned=source.Removedirt()
denoised=cleaned.TemporalDegrain(SAD1=400,SAD2=300,degrain=2)  
left=source
right=denoised
compare= stackhorizontal(subtitle(left,"source",size=28,align=2),subtitle(right,"cleaned",size=28,align=2))
Eval("compare")
Prefetch(8)
