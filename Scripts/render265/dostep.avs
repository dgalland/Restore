#LoadPlugin("../Avisynth/plugins64/GetSystemEnv.dll")
#film = GetSystemEnv("input")
film = "M:\Magis\vegas\1977-1984.avi"
setMemoryMax(8192)
AviSource(film,audio=false)
