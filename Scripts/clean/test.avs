LoadPlugin("../Avisynth/plugins64/mvtools2.dll")
LoadPlugin("../Avisynth/plugins64/masktools2.dll")
LoadPlugin("../Avisynth/plugins64/rgtools.dll")
LoadPlugin("../Avisynth/plugins64/removedirt.dll")
LoadPlugin("../Avisynth/plugins64/fft3Dfilter.dll")
LoadPlugin("../Avisynth/plugins64/hqdn3d.dll")
LoadPlugin("../Avisynth/plugins64/awarpsharp.dll")
LoadPlugin("../Avisynth/plugins64/despot64.dll")
LoadPlugin("../Avisynth/plugins64/neo-fft3d.dll")
LoadPlugin("../Avisynth/plugins64/neo-dfttest.dll")
Import("RemoveDirtMC.avs")
Import("RemoveDirt.avs")
Import("../Avisynth/plugins64/TemporalDegrain.avs")
Import("../Avisynth/plugins64/TemporalDegrain-v2.3.1.avs")

setMemorymax(8192)
#film = "M:\Magis\deshake\38.avi"
film = "M:\Magis\deshake\03.avi"
#film = "M:\Magis\deshake\36.avi"
source= AviSource(film,audio=false).assumefps(16).converttoYV12()
#cleaned_1=source.Removedirt()
cleaned=source.RemovedirtMC(30,false)
#cleaned_2=source.DeSpot(median=false,p1=30, p2=15, pwidth=100, pheight=100, mthres=20,seg=0)
denoised_1=cleaned.TemporalDegrain(SAD1=400,SAD2=300,degrain=3,blksize=16)  
#denoised_1=cleaned.TemporalDegrain(sad1=800,sad2=600,degrain=2,blksize=16)  
denoised_2=cleaned.TemporalDegrain(sad1=800,sad2=600,degrain=3,blksize=16)  
#denoised_1=cleaned.TemporalDegrain(sad1=600,sad2=400,degrain=3,blksize=16)  
#denoised_2=cleaned.TemporalDegrain2(postfft=3,postsigma=3)  
left=denoised_1
right=denoised_2
compare= stackhorizontal(subtitle(left,"source",size=28,align=2),subtitle(right,"cleaned",size=28,align=2))
Eval("compare")
Prefetch(8)
