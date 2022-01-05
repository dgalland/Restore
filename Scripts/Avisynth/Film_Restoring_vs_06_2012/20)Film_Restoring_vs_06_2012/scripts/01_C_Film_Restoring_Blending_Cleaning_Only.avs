# 8mm film restoration script by videoFred.
# www.super-8.be
# info@super-8.be

# version 01.C with frame blending and cleaning but without degraining, for better render speed
# release date: june 20, 2012
#============================================================================================

# august 2010: added removerdirtMC() as suggested by John Meyer
# october 2010: auto sharpening parameters

# march 2011: new autolevels.dll by Jim Battle
# www.thebattles.net/video/autolevels.html

# june 2012: improved stabilising

#=============================================================================================

# cleaning, resizing, stabilizing, sharpening, auto-levels and auto-white balance.
#=============================================================================================




film= "C:\Users\You\Documents\Yourfile.avi"  # source clip, you must specify the full path here




#PARAMETERS
#----------------------------------------------------------------------------------------------------------------------------
result="resultS1" # specify the wanted output here 

trim_begin=2  trim_end= 16  play_speed= 18   #trim frames and play speed (PAL: 16.6666 or 18.75)

final_framerate = 25  # final frame rate

frame_blend= 0.4 # the amount of frame blending


#COLOR AND LEVELS PARAMATERS
#----------------------------------------------------------------------------------------------------------------------------
saturation=1.2   #for all outputs

gamma= 1.2 # for all outputs 

blue= 0  red= 0  #manual color adjustment, when returning result3 or result4. Values can be positive or negative

black_level=0  white_level=235 output_black=0  output_white=255 # manual levels, when returning result4


#AUTO LEVELS PARAMETERS
#--------------------------------------------------------------------------------------------------------------------------------

autolev_low= 4     # limit of autolevels low output
autolev_high= 235  # limit of autolevels high output

 

#SIZE, CROP AND BORDERS PARAMETERS
#----------------------------------------------------------------------------------------------------------------------------
CLeft=40  CTop=40  CRight=40  CBottom=40  #crop values after Depan and before final resizing 

W=720  H=576  #final size after cropping 

bord_left=0  bord_top=0  bord_right=0  bord_bot=0  #720p= borders 150


#STABILISING PARAMETERS, YOU REALY MUST USE RESULTS7 TO CHECK STABILISATION!
#----------------------------------------------------------------------------------------------------------------------------
maxstabH=20 
maxstabV=20 #maximum values for the stabiliser (in pixels) 20 is a good start value

est_left=40   est_top=40  est_right=40  est_bottom=40  #crop values for special Estimate clip

trust_value= 1.0     # scene change detection, higher= more sensitive
cutoff_value= 0.5   # no need to change this, but you can play with it and see what you get




#CLEANING PARAMETERS
#--------------------------------------------------------------------------------------------------------------

dirt_strenght=20  # set this lower for clean films.




# SHARPENING PARAMETERS
#--------------------------------------------------------------------------------------------------------------------------------

USM_sharp_ness= 28   USM_radi_us= 3  #this is the start value for the unsharpmask sharpening
                                     #do not set radius less then 3 
                                     #the script will automatically add two other steps with lower radius 







# END VARIABLES, BEGIN SCRIPT
#=================================================================================================================================


SetMemoryMax(800)  #set this to 1/3 of the available memory


LoadPlugin("plugins/Deflicker.dll")
Loadplugin("plugins/Depan.dll")
LoadPlugin("plugins/DepanEstimate.dll")
Loadplugin("plugins/removegrain.dll")
LoadPlugin("plugins/removedirt.dll")
LoadPlugin("plugins/MVTools.dll")
LoadPlugin("plugins/MVTools2.dll")
Loadplugin("plugins/warpsharp.dll")
LoadPlugin("plugins/autolevels_06.dll")
LoadPlugin("plugins/Motion_06Dec05B.dll")
Import("plugins/03_RemoveDirtMC.avs")




source= AviSource(film).assumefps(play_speed).trim(trim_begin,0).converttoYV12()
trimming= framecount(source)-trim_end
source1= trim(source,0,trimming)





#STABILIZING/CROPPING
#..........................................................................................................................................
stab_reference= source1.crop(20,20,-20,-20).colorYUV(autogain=true).crop(est_left,est_top,-est_right,-est_bottom)

mdata=DePanEstimate(stab_reference,trust=trust_value,dxmax=maxstabH,dymax=maxstabV)
stab=DePanStabilize(source1,data=mdata,cutoff=cutoff_value,dxmax=maxstabH,dymax=maxstabV,method=0,mirror=15).deflicker()
stab2= stab.crop(CLeft,CTop,-CRight,-CBottom)
stab3=DePanStabilize(source1,data=mdata,cutoff=cutoff_value,dxmax=maxstabH,dymax=maxstabV,method=0,info=true)

WS= width(stab)
HS= height(stab)
stab4= stab3.addborders(10,10,10,10,$B1B1B1).Lanczos4Resize(WS,HS)
stab5= Lanczos4Resize(stab2,W,H).sharpen(0.5)


#UNSHARPMASK AUTO_PARAMETERS
#-------------------------------------------------------------------------------------------------------------------------------------------

USM_sharp_ness1 = USM_sharp_ness
USM_sharp_ness2 = USM_sharp_ness+(USM_sharp_ness/2)
USM_sharp_ness3 = USM_sharp_ness*2

USM_radi_us1 = USM_radi_us
USM_radi_us2 = USM_radi_us-1
USM_radi_us3 = USM_radi_us2-1


#CLEANING/PRESHARPENING/RESIZING
#...........................................................................................................................................


noise_baseclip= stab2.levels(0,gamma,255,0,255).tweak(sat=saturation)




cleaned= RemoveDirtMC(noise_baseclip,dirt_strenght).unsharpmask(USM_sharp_ness1,USM_radi_us1,0).unsharpmask(USM_sharp_ness2,USM_radi_us2,0).Lanczos4Resize(W,H)



# FINAL SHARPENING
#.................................................................................................................................................



final_result= cleaned.unsharpmask(USM_sharp_ness3,USM_radi_us3,0)





#RESULT1: AUTOLEVELS,AUTOWHITE
#.................................................................................................................................................
result1= final_result.converttoRGB24().autolevels(output_low= autolev_low, output_high= autolev_high).converttoYV12().coloryuv(autowhite=true).BlendFPS(final_framerate, frame_blend).addborders(bord_left, bord_top, bord_right, bord_bot)

#RESULT2: MANUAL LEVELS, AUTOWHITE
#.................................................................................................................................................
result2= final_result.levels(black_level,1.0,white_level,0,255).coloryuv(autowhite=true).BlendFPS(final_framerate, frame_blend).addborders(bord_left, bord_top, bord_right, bord_bot)


#RESULT3: AUTOLEVELS, MANUAL COLOR CORRECTIONS
#.................................................................................................................................................
result3= final_result.coloryuv(off_U=blue,off_V=red).converttoRGB24().autolevels(output_low= autolev_low, output_high= autolev_high).converttoYV12().BlendFPS(final_framerate, frame_blend).addborders(bord_left, bord_top, bord_right, bord_bot)

#RESULT4: MANUAL LEVELS, MANUAL COLOR CORRECTIONS
#..................................................................................................................................................
result4= final_result.coloryuv(off_U=blue,off_V=red).levels(black_level,1.0,white_level,0,255).BlendFPS(final_framerate, frame_blend).addborders(bord_left, bord_top, bord_right, bord_bot)

#RESULT5: SPECIAL SERVICE CLIP FOR RESULT S5
#..................................................................................................................................................
result5= overlay(source1,greyscale(stab_reference),x=est_left,y=est_top).addborders(2,2,2,2,$FFFFFF).Lanczos4Resize(WS,HS)




#PARAMETERS FOR THE COMPARISONS
#..................................................................................................................................................
W2= W+bord_left+bord_right
H2= H+bord_top+bord_bot





source4=Lanczos4Resize(source1,W2,H2).changeFPS(final_framerate)



#COMPARISONS: ORIGINAL VS RESULTS
#......................................................................................................................................................................
resultS1= stackhorizontal(subtitle(source4,"original",size=28,align=2),subtitle(result1,"result1: autolevels, autowhite",size=28,align=2))
resultS2= stackhorizontal(subtitle(source4,"original",size=28,align=2),subtitle(result2,"result2: autowhite, manual levels correction",size=28,align=2))
resultS3= stackhorizontal(subtitle(source4,"original",size=28,align=2),subtitle(result3,"result3: autolevels, manual color correction",size=28,align=2))
resultS4= stackhorizontal(subtitle(source4,"original",size=28,align=2),subtitle(result4,"result4: manual colors and levels correction",size=28,align=2))
resultS5= stackhorizontal(subtitle(result2,"result2: autowhite, manual levels",size=28,align=2),subtitle(result1,"result1: autowhite, autolevels",size=28,align=2))
resultS6= stackhorizontal(subtitle(result4,"result4: manual levels and color corrections",size=28,align=2),subtitle(result3,"result3: manual color corrections, autolevels",size=28,align=2))

#SPECIAL COMPARISON CLIP FOR TESTING THE STABILIZER
#.........................................................................................................................................................................
resultS7= stackhorizontal(subtitle(result5,"baseclip for stabiliser -only the B/W clip is used",size=32,align=2),subtitle(stab4,"test stabiliser: dx=horizontal, dy=vertical",size=32,align=5)).converttoYUY2()



Eval(result)





