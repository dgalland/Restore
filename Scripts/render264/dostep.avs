LoadPlugin("E:/TOOLS/Avisynth/plugins64/GetSystemEnv.dll")
setMemorymax(8192)
film = GetSystemEnv("input")
SetMemoryMax(8192)
source= AviSource(film,audio=false).assumefps(16).converttoYV12()
Eval("source")
Prefetch(8)
