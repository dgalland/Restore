# film stabilisation script by videoFred.
# modified june 2012
#=============================================================================================




film= "C:\Users\You\Documents\Yourfile.avi"  # source clip, you must specify the full path here




#PARAMETERS
#----------------------------------------------------------------------------------------------------------------------------
result="stabS" # specify the wanted output here stab= stabilised clip, stabS= comparison before/after

trim_begin=2  play_speed= 18   #trim frames and play speed 




#SIZE, CROP AND BORDERS PARAMETERS
#----------------------------------------------------------------------------------------------------------------------------
CLeft=20  CTop=20  CRight=20  CBottom=20  #crop values after Depan and before final resizing 

W=720  H=576  #final size after cropping 

bord_left=0  bord_top=0  bord_right=0  bord_bot=0  #720p= borders 150


#STABILISING PARAMETERS, YOU REALY MUST USE STABS TO CHECK STABILISATION!
#----------------------------------------------------------------------------------------------------------------------------
maxstabH=40 
maxstabV=40 #maximum values for the stabiliser (in pixels) 20 is a good start value

est_left=40   est_top=40  est_right=40  est_bottom=40  #crop values for special Estimate clip

trust_value= 1.0     # scene change detection, higher= more sensitive
cutoff_value= 0.5   # no need to change this, but you can play with it and see what you get




# END VARIABLES, BEGIN SCRIPT
#=================================================================================================================================


SetMemoryMax(800)  #set this to 1/3 of the available memory





Loadplugin("plugins/Depan.dll")
LoadPlugin("plugins/DepanEstimate.dll")




source1= AviSource(film).assumefps(play_speed).trim(trim_begin,0).converttoYV12()




#STABILIZING/CROPPING
#..........................................................................................................................................
stab_reference= source1.crop(est_left,est_top,-est_right,-est_bottom).colorYUV(autogain=true)

mdata=DePanEstimate(stab_reference,trust=trust_value,dxmax=maxstabH,dymax=maxstabV)
stab=DePanStabilize(source1,data=mdata,cutoff=cutoff_value,dxmax=maxstabH,dymax=maxstabV,method=0,mirror=15)
stab2= stab.crop(CLeft,CTop,-CRight,-CBottom)
stab3=DePanStabilize(source1,data=mdata,cutoff=cutoff_value,dxmax=maxstabH,dymax=maxstabV,method=0,info=true)

WS= width(stab)
HS= height(stab)
stab4= stab3.addborders(10,10,10,10,$B1B1B1).Lanczos4Resize(WS,HS)
stab5= Lanczos4Resize(stab2,W,H).sharpen(0.5)







#RESULT5: SPECIAL SERVICE CLIP FOR RESULT S5
#..................................................................................................................................................
result5= overlay(source1,greyscale(stab_reference),x=est_left,y=est_top).addborders(2,2,2,2,$FFFFFF).Lanczos4Resize(WS,HS)




#PARAMETERS FOR THE COMPARISONS
#..................................................................................................................................................
W2= W+bord_left+bord_right
H2= H+bord_top+bord_bot





source4=Lanczos4Resize(source1,W2,H2)





#SPECIAL COMPARISON CLIP FOR TESTING THE STABILIZER
#.........................................................................................................................................................................
stabS= stackhorizontal(subtitle(result5,"baseclip for stabiliser -only the B/W clip is used",size=32,align=2),\
subtitle(stab4,"test stabiliser: dx=horizontal, dy=vertical",size=32,align=5)).converttoYUY2()



Eval(result)




