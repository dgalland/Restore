# Batch Processing avec AviSynth

Ce projet est un ensemble de scripts batch et avisynth pour automatiser la restauration. Les buts sont:

- Faciliter l'installation d'Avisynth et des plugins nécessaire
- Fournir des exemples de scripts avisynth pour les différentes étapes
- Fournir des procédures batch pour automatiser le traitement sans recourir à un GUI

## Les étapes de la restauration

Les étapes d'une restauration sont :

- Join des images capturées pour former le film
- Resize et crop éventuel
- Deshake Stabilisation
- Cleaning Nettoyage 
- Degrain Supression du grain
- Sharpening
- Colors and levels : Couleurs et niveaux

## Avisynth, Virtualdub, codecs

Avisynth est un produit puissant mais dont l'utilisation peut être complexe

- Différentes versions d'Avisynth Avisynth 2.6 Avisynth + 32 bits ou 64 bits multithread ou non
- Difficultés à choisir les bon plugins. Certain plugins sont anciens, non maintenus, disponibles en 32 bits uniquement et nécessitants des versions anciennes des DLL C/C++ Microsoft

Mon expérience montre que l'utilisation d'Avisynth+ 64 apporte la meilleure rapidité et fiabilité. On peut trouver des versions récentes 64bits de tous les plugins nécessaires à la restauration

Si la thread de videofred "The power of Avisynth : restoring old 8mm films.

https://forum.doom9.org/showthread.php?t=144271

reste une référence, son script de restauration est ancien et utilise des plugins obsolètes. En gardant ses étapes il faut le moderniser 

A conseiller "Avisynth Universal installer" qui permet d'installer toutes les versions d'Avisynth avec une procédure batch qui permet de choisir la version à utiliser

Les scripts Avisynth s'exécutent au sein de Virtualdub, installer la version Virtualdub2 64bits

La restauration s'effectuant en plusieurs étapes il faut choisir un codec intermédiaire sans perte.

Les scripts fournis utilisent Magic-Yuv (Commercial Edition standard)  qui donne un très bon résultat.
On peut aussi utiliser UT-Video (Free) mais il faudra alors modifier les templates virtualdub

## Installation des outils

Acheter et installer Magic-Yuv Edition Standard

Ou bien installer le codec UTVideo

https://github.com/umezawatakeshi/utvideo/releases

Les outils nécessaires ne sont pas dans le projet GitHub donc télécharger et installer dans le répertoire tools:

Avisynth Universal installer

https://www.videohelp.com/software/Universal-Avisynth-Installer

Virtualdub2 64 bits

https://sourceforge.net/projects/vdfiltermod/files/latest/download

Downlaod Deshaker plugin 64bits for Virtualdub

http://www.guthspot.se/video/deshaker.htm

ffmpeg 64 bits

https://github.com/BtbN/FFmpeg-Builds/releases

choose win64-gpl.zip

Codec X264 (pour l'encodage final renommer en x264.exe)

https://artifacts.videolan.org/x264/release-win64/

Mkvtoolnix  (mkvmerge pour créer des fichier mkv)

https://www.fosshub.com/MKVToolNix.html

## Installation de avisynth

Ouvrir une fenêtre de commande en mode administrateurDans le répertoire "Avisynth Universal installer", exécuter la procédure setavs et choisir Avisynth + en mode 32 bits et 64 bits

```
cd c:\restore\tools\AvisynthRepository
setavs
```



Dans le répertoire "Avisynth Universal installer", exécuter la procédure setavs et choisir Avisynth + en mode 32 bits et 64 bits

## Conception des scripts

Dans scripts éditer la procédure setup

%TOOLS% 	Localisation du répertoire Tools
%ROOT%   	Localisation du répertoire de vos films

Ouvrir une fenêtre de commande et exécuter la procédure setup

```
cd c:\restore\scripts
setup
```

Tous les exécutables sont appelés une fois pour vérification, il ne doit pas y avoir d'erreur.

Ensuite tous les scripts seront exécuté dans cette fenêtre toujours en restant dans le répertoire scripts

La restauration est conçues en étapes, le résultat de chaque étape devient la source pour l'étape suivante.

Par exemple :

```
do deshake concat 02
```

va exécuter les scripts de deshake  pour le film 02 dans le répertoire concat, le résultat dans le répertoire deshake

Les noms de étapes sont à votre convenance, vous pouvez partir des étapes fournies mais en ajouter d'autres

Vous pouvez dupliquer des étapes pour ajuster les paramètres et faire une comparaison

Chaque sous répertoire du répertoire scripts correspond à une étape et contient:

La procédure  batch de traitement								dostep.cmd
Le script avisynth de restauration									dostep.avs
Le template des settings de virtualdub							template.vdscript
Un script avisynth de test pour ajuster les paramètres 	test.avs

Le répertoire Avisynth contient les plugins nécessaires en 32 et 64 bits

Le projet comporte un répertoire Films pour des essais

Dans par exemple pour la restauration complète du film 01,

```
do join images 01_01 18
do deshake join 01_01
do clean deshake 01_01
do adjust clean 01_01
do render264 adjust 01_01
```

Avec concatenation

```
do concat join 01 01_01 01_02
```

#### Virtualdub processing settings

Pour chaque step le fichier template.vdscript définit les paramètres de Virtualdub  pour le traitement.
Tous les templates sont identiques sauf pour deshake un peu plus compliqué

Pour éditer et modifier les templates fournis

Exécuter Virtualdub64
File->Open video File
	Choose a clip

File->Load Processing settings 
	Choose template.vdscript for the step

Video->Filter
	None except for deshake

Video->Compression 
	Magic-Yuv 4.2.0 
		Configure
			Color Space Rec 709
			Full Range YUV



## Les étapes de la restauration

#### join 

Créer un film à partir des images jpeg

```
do join imagedir clipdir fps [start count]
```

Join des images du clip 01 du film 01 		 												do join images 01_01 18
Join de 200 images du clip 02 du film 01 à partir de l'image 100   		do join images 01_01 18 100 200 

#### Concat

Etape optionnelle concaténer des clips

```
do concat join dest clips
```

exemple:

```
do concat 01 01_01 01_02
```

#### Deshake

Il existe des plugins de deshake pour avisynth. Le plugin deshaker de Virtualdub donne un meilleur résultat pour le traitement des borders qui résultent de la stabilisation:

```
do deshake concat 01
```

ou bien sans concatenation

```
do deshake join 01_01
```

Le traitement  virtualdub est un peu plus complexe car deux passes sont nécessaires, les deux fichiers pass1-template.vdscript et pass2-template.vdscript contiennent les settings de la pass 1 et pass2 du deshaker.

Dans mon cas la capture me donne directement des images à la résolution désirée 1440x1080. Dans le cas contraire virtualdub peut faire du crop/resize mais il vous faudra modifier les deux templates. 

Pour ajuster  les paramètres du deshake éditer dans Virtualdub pass1-template.vdscript et pass2-template.vdscript comme expliqué ci-dessus

#### Clean

```
do clean concat 01
```

Le choix est fait de séparer l'étape de cleaning/denoising car c'est le traitement le plus long. Ainsi on pourra essayer plusieurs étapes d'ajustement sans la recommencer. Ci-dessous le script dostep.avs

```
LoadPlugin("../Avisynth/plugins64/mvtools2.dll")
LoadPlugin("../Avisynth/plugins64/masktools2.dll")
LoadPlugin("../Avisynth/plugins64/rgtools.dll")
LoadPlugin("../Avisynth/plugins64/removedirt.dll")
LoadPlugin("../Avisynth/plugins64/fft3Dfilter.dll")
LoadPlugin("../Avisynth/plugins64/hqdn3d.dll")
LoadPlugin("../Avisynth/plugins64/GetSystemEnv.dll")
Import("RemoveDirt.avs")
Import("../Avisynth/plugins64/TemporalDegrain.avs")

setMemorymax(8192)
film = GetSystemEnv("input")
source= AviSource(film,audio=false)
cleaned=source.Removedirt()
denoised=cleaned.TemporalDegrain(SAD1=400,SAD2=300,degrain=3)  
Eval("denoised")
Prefetch(8)
```

On utilise TemporalDegrain très lent  mais qui donne un bon résultat

Note: Dans tous les scripts avisynth les plugins et leurs paramètres sont ceux qui m'ont donné un bon résultat, vous pouvez bien sûr les modifier. Dans ce cas il peut être intéressant de créer une nouvelle étape, par exemple: clean_stronger pour pouvoir faire la comparaison. Recopier dans le répertoire clean_stronger les scripts de clean, modifier, puis

```
do clean_stronger 01
do compare clean clean_stronger 01
```

#### Adjust

```
do adjust clean 01
```

L'étape adjust consiste en

Sharpening
Correction de niveaux
Correction de couleurs

Dans la version fournie le sharpening est fait avec une recette de VideoFred qui donne un très bon résultat

```
sharpened=source.unsharpmask(30,5,0).blur(0.8).unsharpmask(50,3,0).blur(0.8).sharpen(0.1)
```

Pour les autres corrections c'est plus délicat, cela dépend de votre film.

Il est quasiment impossible de trouver des corrections qui par magie améliorent la totalité du film. Il faut se borner à des corrections assez générales. Les ajustements plus fins par scène devront se faire  dans l'éditeur video.

Regarder clean/dostep.avs  pour les différentes possibilités

#### Changement du fps

Les étapes vont générer un film à 16 ou 18 fps. Des scripts de restauration ajoutent quelquefois un passage en 24fps par duplication de frame ou interpolation.

C'était tout à fait nécessaire pour produire par exemple un DVD.

Je pense que c'est maintenant inutile, les lecteurs video peuvent parfaitement lire un film à 16fps

#### render264

```
do render264 adjust 01
```

Encodage du film en x64 dans container mkv. Encodage en deux passes à débit constant 2000b/s

C'est donné à titre d'exemple, vous pouvez aussi encoder avec virtualdub ou toute sorte d'autres outils.

#### L'éditeur NLE

Pour un résultat parfait le film résultat ne sera pas encodé directement mais traité dans un éditeur NLE

L'éditeur NLE va permettre le découpage, des corrections de couleurs plus fines pour certaines scènes,  l'insertion de titres,  de chapitres etc...

C'est l'éditeur NLE qui assurera l'encodage final

Ici chacun aura son outil préféré, personnellement j'utilise VEGAS Pro dans une version assez ancienne 13

