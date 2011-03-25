#!/bin/bash

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, only version 3 of the License. 
#
# This file is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details. 
#
# You should have received a copy of the GNU General Public License along
# with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
# Author: Valentino C. <skywalkersix@gmail.com>


###six d2x-xl full (cd-iso-demo) install and update script

D2XURL="http://www.descent2.de"
WGETD2XURL="${D2XURL}/downloads"
SVND2XURL="https://d2x-xl.svn.sourceforge.net/svnroot/d2x-xl/trunk"
WGETCMD="wget -c"
USRPREFIX="/usr/local"
D2XROOT="${USRPREFIX}/games/descent2"
CDLOOP="/tmp/loopmount"
D2XSTUFF="D2X-XL-stuff"

# color for echo
EYELLOW='\E[1;33m'
EGREEN='\E[1;32m'
ERED='\E[31m'
EWHITE='\E[1;37m'
EGREY='\E[0;37m'
ERESET="tput sgr0"

# file not found in regular d2x-xl-src archive, but exist in SVN trunk
#SRCWORKAROUNDFILE="main/dialheap.h main/dialheap.cpp"
SRCWORKAROUNDFILE=""

patch_d2x_xl()
{
   case $1 in
   mainmenu.cpp)
cat <<EOF | patch -p1
--- 1/menus/mainmenu.cpp        2010-06-17 16:49:35.000000000 +0200
+++ 2/menus/mainmenu.cpp  2010-06-17 17:49:57.000000000 +0200
@@ -367,10 +367,10 @@
   }
else if (nChoice == mainOpts.nOrder)
   ShowOrderForm ();
-#if defined(_WIN32) || defined(__unix__)
-else if (nChoice == mainOpts.nUpdate)
-   CheckForUpdate ();
-#endif
+//#if defined(_WIN32) || defined(__unix__)
+//else if (nChoice == mainOpts.nUpdate)
+//   CheckForUpdate ();
+//#endif
else
   return 0;
return 1;
EOF
      ;;
   descent.cpp)
cat <<EOF | patch -p1
--- 1/main/descent.cpp  2010-06-17 16:49:44.000000000 +0200
+++ 2/main/descent.cpp    2010-06-17 17:46:10.000000000 +0200
@@ -775,7 +775,7 @@
InitGameOptions (0);
InitArgs (argc, argv);
EvalArgs ();
-CheckAndFixSetup ();
+//CheckAndFixSetup ();
GetAppFolders ();
if (FindArg ("-debug-printlog") || FindArg ("-printlog")) {
      char fnErr [FILENAME_LEN];
EOF
      ;;
   esac
}

wget_addons_base()
{
   # d2x-xl source for linux and d2x-xl-data + bin for windows (base/pure install)
   cd ${D2XROOT}/${D2XSTUFF}/downloads/d2x-xl-data
   echo > ../../status/wgetlog/base
   # found installed version
   INUSEVER="$(ls -1 d2x-xl-src-* | grep -iv current | tail -n1 | cut -d"-" -f4 | cut -d"." -f -3)"
   [ $INUSEVER != ${ARRAYVER[2]} ] && echo "$INUSEVER" > ../../status/inusever
   # download current version
   script -qac "$WGETCMD ${WGETD2XURL}/d2x-xl-win-${ARRAYVER[2]}.exe" ../../status/wgetlog/base
   script -qac "$WGETCMD ${WGETD2XURL}/d2x-xl-src-${ARRAYVER[2]}.7z" ../../status/wgetlog/base
   rm -f $(find . -type l)
   ln -s d2x-xl-win-${ARRAYVER[2]}.exe d2x-xl-win-current.exe
   ln -s d2x-xl-src-${ARRAYVER[2]}.7z d2x-xl-src-current.7z
}

wget_addons_allgoodies()
{
   # hires models
   cd ${D2XROOT}/${D2XSTUFF}/downloads/models
   echo > ../../status/wgetlog/models
   MODELS="   ship-pyrogl.7z         ship-phantomxl.7z
         gun-laser.7z         gun-superlaser.7z
         gun-spreadfire.7z      gun-helix.7z
         gun-plasma.7z         gun-phoenix.7z
         gun-fusion.7z         gun-omega.7z
         gun-vulcan.7z         gun-gauss.7z
         msl-concussion.7z      msl-flash.7z
         msl-homing.7z         msl-guided.7z
         msl-smart.7z         msl-mercury.7z
         msl-mega.7z         msl-eshaker.7z
         mine-smart.7z         mine-proxy.7z
         dev-afterburner.7z      dev-converter.7z
         dev-headlight.7z      dev-ammorack.7z
         dev-fullmap.7z         dev-quadlaser.7z
         dev-vulcanammo.7z      dev-slowmotion.7z
         dev-bullettime.7z      dev-keys.7z
         hostage.7z         bullet.7z"
   for RAR in $MODELS
   do
         script -qac "$WGETCMD ${WGETD2XURL}/models/$RAR" ../../status/wgetlog/models
   done
   # hires textures d2
   cd ../textures
   echo > ../../status/wgetlog/textures
   TEXTURES="   D2-hires-ceilings.7z      D1-hires-ceilings.7z
         D2-hires-doors.7z      D1-hires-doors.7z
         D2-hires-fans-grates.7z      D1-hires-fans-grates.7z
         D2-hires-lava-water.7z      D1-hires-lava.7z
         D2-hires-lights.7z      D1-hires-lights.7z
         D2-hires-metal.7z      D1-hires-metal.7z
         D2-hires-rock.7z      D1-hires-rock.7z
         D2-hires-signs.7z      D1-hires-signs.7z
         D2-hires-special.7z      D1-hires-special.7z
         D2-hires-switches.7z      D1-hires-switches.7z
         hires-reticles.7z
         hires-keys.7z
         hires-powerups.7z
         hires-effects.7z
         hires-robot-textures.7z
         hires-missiles.7z"
   for RAR in $TEXTURES
   do
         script -qac "$WGETCMD ${WGETD2XURL}/textures/$RAR" ../../status/wgetlog/textures
   done
   # complete level pack
   cd ../levels
   echo > ../../status/wgetlog/levelpack
   script -qac "$WGETCMD ${WGETD2XURL}/levelpack.7z" ../../status/wgetlog/levelpack
   # sound and music D1 and D2

   cd ../sounds
   echo > ../../status/wgetlog/sounds
   script -qac "$WGETCMD ${WGETD2XURL}/sound/hires-sounds.7z" ../../status/wgetlog/sounds
   script -qac "$WGETCMD ${WGETD2XURL}/sound/8mbgm_enhanced18.7z" ../../status/wgetlog/sounds
   # mods and conversions
   cd ../mods
   echo > ../../status/wgetlog/mods
   MODS="      D2-Mod-1.7z      D1-Quake-Mod.7z
         D2-Mod-2.7z      Weapon-and-Effects-Mod.7z"
   for RAR in $MODS
   do
         script -qac "$WGETCMD ${WGETD2XURL}/mods/$RAR" ../../status/wgetlog/mods
   done
   #Custom Menu Backgrounds
   cd ../menu-background
   echo > ../../status/wgetlog/background
   script -qac "$WGETCMD ${WGETD2XURL}/wallpapers/d2x-xl-menubg-1.7z" ../../status/wgetlog/background
   script -qac "$WGETCMD ${WGETD2XURL}/wallpapers/d2x-xl-menubg-2.7z" ../../status/wgetlog/background
}

wget_addons_demo()
{
   cd ${D2XROOT}/${D2XSTUFF}/downloads/d2demo
   echo > ../../status/wgetlog/demo
   script -qac "$WGETCMD ${WGETD2XURL}/D2-demodata.7z" ../../status/wgetlog/demo
}

svn_src_download()
{
   cd ${D2XROOT}/${D2XSTUFF}/SVNbuild
   rm -fR trunk
   svn co $SVND2XURL
}

download_only()
{
   case $1 in
   base)
      wget_addons_base
      ;;
   full)
      wget_addons_base
      wget_addons_allgoodies
      ;;
   demo)
      wget_addons_demo
      ;;
   basedemo)
      wget_addons_base
      wget_addons_demo
      ;;
   fulldemo)
      wget_addons_base
      wget_addons_allgoodies
      wget_addons_demo
      ;;
   esac
}

sub_create_backup()
{
   cd ${D2XROOT}
   tar -vczf $CURRDIR/${D2XSTUFF}.tar.gz --exclude=*build --exclude=status ${D2XSTUFF}
}

sub_restore_backup()
{
   if [ -f $CURRDIR/${D2XSTUFF}.tar.gz ] ; then
      mkdir -p ${D2XROOT}
      tar -vxzf $CURRDIR/${D2XSTUFF}.tar.gz -C ${D2XROOT}
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t$CURRDIR/${D2XSTUFF}.tar.gz file not found!"
   fi
}

read_prompt()
{
   # input match check
   # $1=default value, $n=choices value
   while true
   do
      echo -e ${EGREEN}
      case $# in
      3)
         read -p " [$2|$3] > " ANSWER
         ;;
      4)
         read -p " [$2|$3|$4] > " ANSWER
         ;;
      5)
         read -p " [$2|$3|$4|$5] > " ANSWER
         ;;
      6)
         read -p " [$2|$3|$4|$5|$6] > " ANSWER
         ;;
      esac
      [ "0$ANSWER" = "0" ] && ANSWER="$1"
      if [ "$ANSWER" = "$2" -o "$ANSWER" = "$3" -o "$ANSWER" = "$4" -o "$ANSWER" = "$5" -o "$ANSWER" = "$6" ] ; then
         break 2
      else
         echo -e "${ERED}\n\tSorry, response >$ANSWER< not understood."
      fi
   done
   $ERESET
}

check_prompt()
{
   if [ "$1" != "$2" -a "$1" != "$3" -a "$1" != "$4" -a "$1" != "$5" -a "$1" != "$6" ] ; then
      echo -e "${ERED}\n\tSorry, response >$1< not understood."
      read_prompt $@
   else
      ANSWER="$1"
   fi
}

install_addons_base()
{
   cd ${D2XROOT}/${D2XSTUFF}
   grep -qs 'd2x-xl-src-' /tmp/d2x-update.log
   if [ "0$?" = "00" -o "$UPDATECK" = "no" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tInstall or Update d2x-xl base\n"
      sleep 1
      rm -fR build
      mkdir -p build
      [ "$1" != "rebuild" ] && unrar x -y downloads/d2x-xl-data/d2x-xl-win-current.exe ${D2XROOT}/
      7z x -y -o./build/ downloads/d2x-xl-data/d2x-xl-src-current.7z
      cd build
      7z x -y ./d2x-xl-makefiles.7z
      chmod 0755 ./autogen.sh ./configure
   fi
}

install_addons_allgoodies()
{
   cd ${D2XROOT}/${D2XSTUFF}/downloads
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tInstall or Update d2x-xl allgoodies"
   sleep 1
   for DIRR in models textures mods levels sounds
   do
      for RAR in ./$DIRR/*.7z
      do
         grep -qs "$(basename $RAR)" /tmp/d2x-update.log
         if [ "0$?" = "00" -o "$UPDATECK" = "no" ] ; then
            7z x -y -o../../ $RAR
         fi
      done
   done
   # fix wrong *.tga path
   cd ../../models
   sed -i -e "sY.*\\\Y\t\t\t\*BITMAP \"Yg" *.ase
   cd - > /dev/null 2>&1
}

check_iso()
{
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tInstall the game directly from the CD is not recommended, because the Mixed Mode CDs \
   \n\tare often the cause of problems. To avoid any kind of problem with mount or emulation (DOSBox) \
   \n\tand the format of the data contained in the CD, D2X-XL-install automatically generate the ISO \
   \n\tfrom which we will run the setup, in a session DOSBox"
   if [ -s ${D2XROOT}/${D2XSTUFF}/ISO/$1.iso ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\t$1.iso found in the path ${D2XSTUFF}/ISO, try installing from that? -> default yes"
      read_prompt yes yes no
   fi
   if [ "$ANSWER" = "no" -o ! -s ${D2XROOT}/${D2XSTUFF}/ISO/$1.iso ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tcreation of the ${D2XSTUFF}/ISO/$1.iso Track 1 data from CD\n"
      $ERESET
      create_iso_from_cd $1
   fi
}

create_iso_from_bin()
{
   LINEPATH="$(dirname $1)"
   LINEFILECUE="$(basename $1)"
   LINEFILE="$(echo $LINEFILECUE | sed -e "s/.cue$//" -e "s/.CUE$//")"
   if [ -f ${LINEPATH}/${LINEFILE}.bin -o -f ${LINEPATH}/${LINEFILE}.BIN ] ; then
      cd $LINEPATH
      bin2iso $LINEFILECUE
      mv -fv ${LINEFILE}-01.iso ${D2XROOT}/${D2XSTUFF}/ISO/$2.iso
      cd - > /dev/null 2>&1
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t${LINEPATH}/${LINEFILE}.bin not found!\n\t.cue and .bin files must be in the same directory"
   fi
}

create_iso_from_cd()
{
   REQCD="$1"
   # find the cdrom in the computer
   CDLIST=($(cdrecord --devices -s | egrep -v -i "virt|----|wodim:" | cut -d"'" -f2))
   for CD in $CDLIST
   do
      # find the label of current cdrom
      CDLABEL=$(cd-info --no-device-info -T -C $CD | tr -d "[:blank:]" | egrep -i "volume:" | cut -d":" -f2)
      # check if the current cdrom is that required
      if [ "$CDLABEL" = "$REQCD" ] ; then
         TRACKRANGE=($(cd-info --no-device-info -C $CD | tr -s "[:blank:]" " " | egrep -i " 1:| 2:" | cut -d" " -f4))
         readcd dev=$CD f=${D2XROOT}/${D2XSTUFF}/ISO/${CDLABEL}.iso sectors=${TRACKRANGE[0]}-${TRACKRANGE[1]}
         echo
         touch ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid
      fi
   done
   [ -f ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid ] || echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t$1 CD not found! try fallback to ${D2XSTUFF}/ISO/$1.iso"
}

install_d1()
{
   cd ${D2XROOT}/${D2XSTUFF}/ISO
   if [ -f DESCENT.iso ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDESCENT setup with DOSBox: Leave all the setup options to default values (just hit enter) \
      \n\texcept for the question '${EGREEN}shareware version of descent${EWHITE}' -> answer >${EGREEN}don't copy data file${EWHITE}< \
      \n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
      sleep 3
      $ERESET
      mount -o loop DESCENT.iso $CDLOOP > /dev/null 2>&1
      chmod o+w ${USRPREFIX}/games
      su $GUESSEDUSER -c "dosbox -c \"mount c ${USRPREFIX}\" -c \"mount d $CDLOOP -t cdrom -label DESCENT\" -c \"d:\" -c \"install.exe\" -c \"exit\"" > /dev/null 2>&1
      # level is already in dlotw and newlevel is already in descent
      cp -af ${CDLOOP}/dlotw/* ${USRPREFIX}/games/DESCENT/
      chmod o-w ${USRPREFIX}
      touch ../status/d1.pid
      umount $CDLOOP
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tdid not find any >CDrom< or >${D2XSTUFF}/ISO/DESCENT.iso< ! Setup can not continue!\n"
      $ERESET
      exit 1
   fi
}

install_d2()
{
   cd ${D2XROOT}/${D2XSTUFF}/ISO
   if [ -f DESCENT_II.iso ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDESCENT_II setup with DOSBox: Leave all the setup options to default values (just hit enter) \
      \n\texcept for the question '${EGREEN}do you whish to continue${EWHITE}' -> answer >${EGREEN}yes${EWHITE}< \
      \n\tand make sure that the '${EGREEN}installation choices${EWHITE}' is >${EGREEN}crazy${EWHITE}< \
      \n\tand resolution of movies is >${EGREEN}high${EWHITE}< \
      \n\tFor the question '${EGREEN}S3 version of D2${EWHITE}' -> answer >${EGREEN}no${EWHITE}< \
      \n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
      sleep 3
      $ERESET
      mount -o loop DESCENT_II.iso $CDLOOP > /dev/null 2>&1
      chmod -R o+w ${D2XROOT}
      su $GUESSEDUSER -c "dosbox -c \"mount c ${USRPREFIX}\" -c \"mount d $CDLOOP -t cdrom -label DESCENT_II\" -c \"d:\" -c \"install.exe\" -c \"exit\"" > /dev/null 2>&1
      chmod -R o-w ${D2XROOT}
      touch ../status/d2.pid
      umount $CDLOOP
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tdid not find any >CDrom< or >${D2XSTUFF}/ISO/DESCENT_II.iso< ! Setup can not continue!\n"
      $ERESET
      exit 1
   fi
}

install_d2_vertigo()
{
   cd ${D2XROOT}/${D2XSTUFF}/ISO
   if [ -f D2_VERTIGO.iso ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tD2_VERTIGO setup with DOSBox: Leave all the setup options to default values (just hit enter) \
      \n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
      sleep 3
      $ERESET
      [ -f ../../descent2.exe ] || cp -af ../status/backup4d2_vertigo/* ../../
      mount -o loop D2_VERTIGO.iso $CDLOOP > /dev/null 2>&1
      chmod -R o+w ${D2XROOT}
      su $GUESSEDUSER -c "dosbox -c \"mount c ${USRPREFIX}\" -c \"mount d $CDLOOP -t cdrom -label D2_VERTIGO\" -c \"d:\" -c \"install.exe\" -c \"exit\"" > /dev/null 2>&1
      chmod -R o-w ${D2XROOT}
      touch ../status/d2_vertigo.pid
      umount $CDLOOP
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tdid not find any >CDrom< or >${D2XSTUFF}/ISO/D2_VERTIGO.iso< ! Setup can not continue!\n"
      $ERESET
      exit 1
   fi
}

install_d2_demo()
{
   cd ${D2XROOT}/${D2XSTUFF}/downloads/d2demo
   7z x -y -o../../../data/ D2-demodata.7z
}

fix_perm_fix_lowercase()
{
   cd ${D2XROOT}
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tSet the owner, permissions and lowercase"
   # change the owner of all files in root
   chown -R root:root $(ls -A) 2> /dev/null

   # rename all lower case, leave out stuff directory
   find . -depth | egrep -v "${D2XSTUFF}| " | while read LINE
   do
      LINEPATH="$(dirname $LINE)"
      LINEFILE="$(basename $LINE)"
      [ -f $LINE ] && chmod 644 $LINE
      [ -d $LINE ] && chmod 755 $LINE
      mv -n "$LINE" "${LINEPATH}/$(echo $LINEFILE | tr "[:upper:]" "[:lower:]")" 2> /dev/null
   done
   [ -f d2x-xl ] && chmod 0755 d2x-xl
}

move_to_newpath()
{
   cd ${D2XROOT}
   mkdir -p config demos data profiles movies
   [ "$1" = "DESCENT" ] && cd ../DESCENT

   # move the original files in the new path
   mv -f *.{cfg,CFG} *.{ini,INI} ${D2XROOT}/config/ > /dev/null 2>&1
   mv -n *.{dem,DEM} ${D2XROOT}/demos/ > /dev/null 2>&1
   mv -f *.{hog,HOG} *.{msn,MSN} *.{ham,HAM} *.{pig,PIG} *.{s??,S??} ${D2XROOT}/data/ > /dev/null 2>&1
   mv -n *.{plr,PLR} *.{plx,PLX} ${D2XROOT}/profiles/ > /dev/null 2>&1
   mv -n *.{mvl,MLV} ${D2XROOT}/movies/ > /dev/null 2>&1
}

build_install_d2x_xl()
{
   if [ "$1" = "svn" ] ; then
      cd ${D2XROOT}/${D2XSTUFF}/SVNbuild/trunk
   else
      cd ${D2XROOT}/${D2XSTUFF}/build
   fi
   if [ -f ./autogen.sh -a -f ./configure ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tBuild and Install d2x-xl executable\n"
      sleep 1
      # Dowload missing src file
      for FILE in $SRCWORKAROUNDFILE
      do
         if [ ! -f $FILE ] ; then
            cd $(dirname $FILE)
            svn export ${SVND2XURL}/$FILE
            cd - > /dev/null 2>&1
            [ -f $FILE ] || echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tDownload of $FILE failed!!"
         fi
      done
      sh ./autogen.sh
      ./configure --enable-release=yes --enable-debug=no
      J=$(( $(egrep "^processor" /proc/cpuinfo | tail -n1 | cut -d":" -f2) + 2 ))
      [ "$J" -le "2" ] && J="1"
      while true
      do
         make -j $J 2> ${D2XROOT}/${D2XSTUFF}/status/d2x-xl-make.log
         if [ -f ./d2x-xl ] ; then
            [ -f ../../d2x-xl ] && mv ../../d2x-xl ../../d2x-xl-${INUSEVER}.backup
            cp -af d2x-xl ../../
            touch ${D2XROOT}/${D2XSTUFF}/status/build.pid
            break 2
         else
            # find files to patch
            PATCHF="$(grep -i "undefined reference to" ${D2XROOT}/${D2XSTUFF}/status/d2x-xl-make.log | head -n1 | cut -d":" -f1)"
            if [ "0$PATCHF" != "0" ] ; then
               patch_d2x_xl $PATCHF
            else
               echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tbuild failed! probably the problem will be fixed in the next version, \
               \n\tjust wait few days and try again with '$0 --d2x-xl-update'\n"
               break 2
            fi
         fi
      done
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tno src to build!\n"
   fi
}

sub_cd_d1()
{
   if [ "0$1" = "0" ] ; then
      check_iso DESCENT
   else
      if [ -f $1 ] ; then
         echo $1 | grep -qi ".cue"
         if [ "0$?" = "00" ] ; then
            create_iso_from_bin $1 DESCENT
         else
            echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tcopy .iso in the ${D2XSTUFF}/ISO path"
            cp -afv $1 ${D2XROOT}/${D2XSTUFF}/ISO/DESCENT.iso
         fi
      else
         echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t$1 file not exist!!"
         $ERESET
         exit 1
      fi
   fi
   $ERESET
   install_d1
   move_to_newpath DESCENT
   fix_perm_fix_lowercase
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDo you want to install the 'DESCENT II' CD ? -> default yes \
   \n\tyou can also specify a .iso previously created (not for those already in ${D2XSTUFF}/ISO), \
   \n\tlike command switch --cd-d2"
   read_prompt yes yes no iso cue
   case $ANSWER in
   yes|"")
      sub_cd_d2
      ;;
   no)
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDo you want to continue with the installation of D2X-XL ? -> default yes"
      read_prompt yes yes no
      [ "$ANSWER" = "yes" ] && sub_d2x_xl
      ;;
   iso)
      echo -e ${EGREEN}
      read -p " [/path/to/d2cd_track1data.iso] > " OWNISO
      sub_cd_d2 $OWNISO
      ;;
   cue)
      echo -e ${EGREEN}
      read -p " [/path/to/d2cd.cue] > " OWNISO
      sub_cd_d2 $OWNISO
      ;;
   esac
}

sub_cd_d2()
{
   if [ "0$1" = "0" ] ; then
      check_iso DESCENT_II
   else
      if [ -f $1 ] ; then
         echo $1 | grep -qi ".cue"
         if [ "0$?" = "00" ] ; then
            create_iso_from_bin $1 DESCENT_II
         else
            echo -e "${EYELLOW}D2X-XL-install::${EWHITE}\n\tcopy .iso in the ${D2XSTUFF}/ISO path"
            cp -afv $1 ${D2XROOT}/${D2XSTUFF}/ISO/DESCENT_II.iso
         fi
      else
         echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t$1 file not exist!!"
         $ERESET
         exit 1
      fi
   fi
   $ERESET
   install_d2
   fix_perm_fix_lowercase
   move_to_newpath
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tdo you want to install the 'D2 VERTIGO' CD ? -> default yes \
   \n\tyou can also specify a .iso previously created (not for those already in ${D2XSTUFF}/ISO), \
   \n\tlike command switch --cd-d2-vertigo"
   read_prompt yes yes no iso cue
   case $ANSWER in
   yes|"")
      sub_cd_d2_vertigo
      ;;
   no)
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDo you want to continue with the installation of D2X-XL ? -> default yes"
      read_prompt yes yes no
      [ "$ANSWER" = "yes" ] && sub_d2x_xl
      ;;
   iso)
      echo -e ${EGREEN}
      read -p " [/path/to/d2vertigocd_track1data.iso] > " OWNISO
      sub_cd_d2_vertigo $OWNISO
      ;;
   cue)
      echo -e ${EGREEN}
      read -p " [/path/to/d2vertigocd.cue] > " OWNISO
      sub_cd_d2_vertigo $OWNISO
      ;;
   esac
}

sub_cd_d2_vertigo()
{
   if [ "0$1" = "0" ] ; then
      check_iso D2_VERTIGO
   else
      if [ -f $1 ] ; then
         echo $1 | grep -qi ".cue"
         if [ "0$?" = "00" ] ; then
            create_iso_from_bin $1 D2_VERTIGO
         else
            echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tcopy .iso in the ${D2XSTUFF}/ISO path"
            cp -afv $1 ${D2XROOT}/${D2XSTUFF}/ISO/D2_VERTIGO.iso
         fi
      else
         echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t$1 file not exist!!"
         $ERESET
         exit 1
      fi
   fi
   $ERESET
   install_d2_vertigo
   fix_perm_fix_lowercase
   move_to_newpath
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tDo you want to continue with the installation of D2X-XL ? -> default yes"
   read_prompt yes yes no
   [ "$ANSWER" = "yes" ] && sub_d2x_xl
}

sub_d2x_xl()
{
   if [ "0$1" = "0" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tWhich type of d2x-xl installation you want? -> default full\n"
      read_prompt full base full
   else
      check_prompt $1 base full
   fi
   echo -e "$ANSWER\n" > ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype
   UPDATECK="no"
   download_only $ANSWER
   if [ ! -f ${D2XROOT}/data/descent.hog -a ! -f ${D2XROOT}/data/descent2.hog -a ! -f ${D2XROOT}/data/d2demo.hog ] ; then
      wget_addons_demo
      install_d2_demo
      echo -e "${ANSWER}demo\n" > ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype
   fi
   install_addons_base
   # cp -afv ${D2XROOT}/config/d2x-default.ini ${D2XROOT}/config/d2x.ini
   [ "$ANSWER" = "full" ] && install_addons_allgoodies
   build_install_d2x_xl
   if [ -f ${D2XROOT}/${D2XSTUFF}/status/build.pid ] ; then
      finalize_installation
   else
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tdo you want download and build the SVN version? -> default no"
      read_prompt no yes no
      case $ANSWER in
      yes)
         svn_src_download
         build_install_d2x_xl svn
         if [ -f ${D2XROOT}/${D2XSTUFF}/status/build.pid ] ; then
            finalize_installation
         else
            finalize_installation onlyclean
         fi
         ;;
      no)
         finalize_installation onlyclean
         ;;
      esac
   fi
}

sub_d2x_xl_update()
{
   if [ "$1" = "rebuild" ] ; then
      UPDATECK="no"
      install_addons_base rebuild
      build_install_d2x_xl
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tOk! go to desktop menu and launch the game!\n\tor.., \
      \n\tOk! from an user shell, run ${D2XROOT}/d2x-xl to launch the game!\n"
   else
      D2XINSTTYPE="$(cat ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype)"
      [ "0$D2XINSTTYPE" = "0" ] && D2XINSTTYPE="full"
      UPDATECK="yes"
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tCheck and download updates...\n"
      $ERESET
      download_only $D2XINSTTYPE
      egrep "^[0-9]{4}-[0-9]{2}-[0-9]{2}" ${D2XROOT}/${D2XSTUFF}/status/wgetlog/* | tr -d '"' | cut -d" " -f6 | grep -vi found > /tmp/d2x-update.log
      install_addons_base
      [ "$D2XINSTTYPE" = "full" -o "$D2XINSTTYPE" = "fulldemo" ] && install_addons_allgoodies
      grep -qs 'd2x-xl-src-' /tmp/d2x-update.log
      if [ "0$?" = "00" ] ; then
         build_install_d2x_xl
         if [ -f ${D2XROOT}/${D2XSTUFF}/status/build.pid ] ; then
            finalize_installation update
         else
            echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tdo you want download and build the SVN version? -> default no"
            read_prompt no yes no
            case $ANSWER in
            yes)
               svn_src_download
               build_install_d2x_xl svn
               if [ -f ${D2XROOT}/${D2XSTUFF}/status/build.pid ] ; then
                  finalize_installation update
               else
                  finalize_installation onlyclean
               fi
               ;;
            no)
               finalize_installation onlyclean
               ;;
            esac
         fi
      else
         finalize_installation onlyclean
      fi
   fi
}

sub_stuff_download()
{
   if [ "0$1" = "0" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tWhich type of d2x-xl download you want? -> default full"
      read_prompt full base full demo
   else
      check_prompt $1 base full demo
   fi
   download_only $ANSWER
}

create_menu_entry()
{
   if [ ! -f //d2x-xl.desktop ] ; then
      echo -e "[Desktop Entry]\nEncoding=UTF-8\nExec=${D2XROOT}/d2x-xl\nIcon=${D2XROOT}/d2x-xl-0.png\nType=Application \
      \nTerminal=false\nName=D2X-XL\nGenericName=Six degrees of freedom FPS\nStartupNotify=true \
      \nCategories=Game;ActionGame;\n" > /usr/share/applications/d2x-xl.desktop
      chmod 644 /usr/share/applications/d2x-xl.desktop
      convert ${D2XROOT}/d2x-xl.ico ${D2XROOT}/d2x-xl.png
   fi
}

home_user_config()
{
   # for linux user
   PS3="next user number: > "
   echo -e ${EGREEN}
   select LU in $(getent passwd | cut -d":" -f 1,6 | grep "/home" | cut -d":" -f1) Done
   do
      [ "$LU" = "Done" ] && break
      if [ ! -f users/$LU ] ; then
         mkdir -p /home/$LU/.d2x-xl   # /{config,profiles,savegames,screenshots,demos,cache}
         cp -af ${D2XROOT}/{config,profiles,savegames,screenshots,demos,cache} /home/$LU/.d2x-xl/
         dos2unix -b -U /home/$LU/.d2x-xl/config/*
         chown -R $LU:users /home/$LU/.d2x-xl
         touch ${D2XROOT}/${D2XSTUFF}/status/users/$LU
         echo -e "${EWHITE}\n\tuser ${EGREY}$LU ${EWHITE}is done."                                                                                   
      else                                                                                                                                                 
         echo -e "${EWHITE}\n\tusers already configured:${EGREY} $(ls ./users | tr "\n" "," | sed -e "s/,/, /g")"                                     
      fi
      echo -e ${EGREEN}
   done
   PS3="> "
}

finalize_installation()
{
   fix_perm_fix_lowercase
   do_clean
   if [ "$1" = "onlyclean" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tin the next time, to retry do not forget to use '$0 --d2x-xl-update'"
   else
      if [ "$1" = "update" ] ; then
         echo -e "\n${EYELLOW}D2X-XL-update::${EWHITE}\n\tlist of users already configured: \
         \n${EGREY}$(ls ${D2XROOT}/${D2XSTUFF}/status/users | tr "\n" "," | sed -e "s/,/, /g")\n\t${EWHITE}do you want add more user? -> default no"
         read_prompt no yes no
         [ "$ANSWER" = "yes" ] && home_user_config
      else
         echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tD2x-xl in linux keeps profiles, saved games, screenshots, etc.. of different users \
         \n\tin their home directory.\n\tWhich users must be configured?"
         home_user_config
      fi
      create_menu_entry
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tOk! go to desktop menu and launch the game!\n\tor.., \
      \n\tOk! from an user shell, run ${D2XROOT}/d2x-xl to launch the game!\n"
   fi
}

sanity_check()
{
   if [ "0$(id -u)" != "00" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tThis script should be run as root!!!\n"
      $ERESET
      exit 1
   fi
   echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\tSanity check...\n"
   # check the current user login in graphic mode, will be used by DOSBox
   GUESSEDUSER="$(who -u | cut -d" " -f1 | head -n1)"
   if [ "0$GUESSEDUSER" = "0" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}\n\t$0: did not found the real owner of the X session, to run dosbox. \
      \n\tplease type the name of X login user, or the user that owns the X session\n"
      echo -e ${EGREEN}
      read -p " X login username: > " GUESSEDUSER
   fi
   # check free space
   mkdir -p ${D2XROOT} $CDLOOP
   SIZES=($(stat -L -f -c "%a %S" ${D2XROOT}))
   FREES=$((${SIZES[0]}*${SIZES[1]}))
   FREESMB=$(($FREES/1024/1024))
   [ $FREES -lt 5700000000 ] && echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t!! ATTENTION !!\n\tif you plan to do a >full< install, \
   \n\tyou need at least >5600< MB of free space\n\tand now you have $FREESMB MB free on ${USRPREFIX}/games"
   # check dependencies
   MISSPKG=""
   for DEP in wget unrar 7z dos2unix dosbox make gcc nasm autoconf automake convert cdrecord cd-info bin2iso sdl-config svn patch
   do
      which $DEP > /dev/null 2>&1
      if [ "0$?" != "00" ] ; then
         case $DEP in
         cd-info)
            DEPPKG="libcdio"
            ;;
         sdl-config)
            DEPPKG="SDL"
            ;;
         convert)
            DEPPKG="imagemagick"
            ;;
         svn)
            DEPPKG="subversion"
            ;;
         7z)
            DEPPKG="p7zip"
            ;;
         dos2unix)
            DEPPKG="hd2u"
            ;;
         *)
            DEPPKG="$DEP"
            ;;
         esac
         MISSPKG="$MISSPKG $DEPPKG"
      fi
   done
   for DEP in SDL_mixer SDL_image libGLU.so libXm.so libGLEW.so
   do
      ldconfig -p | grep -q $DEP
      if [ "0$?" != "00" ] ; then
         case $DEP in
         libGLU.so)
            DEPPKG="Mesa"
            ;;
         libXm.so)
            DEPPKG="'OpenMotif or lesstif'"
            ;;
         libGLEW.so)
            DEPPKG="glew"
            ;;
         *)
            DEPPKG="$DEP"
            ;;
         esac
         MISSPKG="$MISSPKG $DEPPKG"
      fi
   done
   if [ "0$MISSPKG" != "0" ] ; then
      echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\tMissing dependencies: $MISSPKG\n\t${EWHITE}Install the missing packages and try again...\n"
      $ERESET
      exit 1
   fi
   # find current vesion and test internet connection
   ARRAYVER=($(wget -O - -q ${D2XURL}/d2x.html | tr -s "<>" "\n" | grep -i "current version"))
   [ "0${ARRAYVER[2]}" = "0" ] && echo -e "\n${EYELLOW}D2X-XL-install::${ERED}\n\t!! ATTENTION !!\n\tNO INTERNET CONNECTION!! if you plan to do a new D2X-XL installation, \
   \n\tmake sure you have a backup a previous istallation...\n"
   # create directory structure
   cd ${D2XROOT}
   mkdir -p cache movies savegames screenshots mods ${D2XSTUFF}/{build,SVNbuild,ISO} ${D2XSTUFF}/status/users \
   ${D2XSTUFF}/downloads/{d2demo,d2x-xl-data,levels,menu-background,models,mods,sounds,textures}
   [ -h ../d2x-xl ] || ln -s ./descent2 ../d2x-xl 2> /dev/null
   $ERESET
}

do_clean()
{
   cd ${D2XROOT}
   INUSEVER="$(cat ${D2XSTUFF}/status/inusever)"
   # delete unnecessary files
   if [ -f ${D2XSTUFF}/status/d2_vertigo.pid ] ; then
      rm -fR descent2.exe ${D2XSTUFF}/status/backup4d2_vertigo
   else
      mkdir -p ${D2XSTUFF}/status/backup4d2_vertigo
      mv descent2.exe ${D2XSTUFF}/status/backup4d2_vertigo/
   fi
   rm -fR ../DESCENT d2.bat descent.cfg eregcard.exe hmi*.386 setup.exe ${D2XSTUFF}/status/wgetlog/*
   grep -qs 'd2x-xl-src-' /tmp/d2x-update.log
   if [ "0$?" = "00" -o "$UPDATECK" = "no" -a -f ${D2XSTUFF}/status/build.pid ] ; then
      rm -fR ${D2XSTUFF}/build/* ${D2XSTUFF}/SVNbuild/trunk/*
      rm -f $(ls -1 d2x-xl-*.backup | egrep -v "${ARRAYVER[2]}|$INUSEVER")
      rm -f $(ls -1 ${D2XSTUFF}/downloads/d2x-xl-data/d2x-xl-*.{exe,7z} | egrep -v "${ARRAYVER[2]}|$INUSEVER|current")
   fi
}

###-MAIN-###-MAIN-###-MAIN-###

CURRDIR="$(pwd)"

case $1 in
--cd-d1)
   sanity_check
   sub_cd_d1 $2
   ;;
--cd-d2)
   sanity_check
   sub_cd_d2 $2
   ;;
--cd-d2-vertigo)
   sanity_check
   sub_cd_d2_vertigo $2
   ;;
--d2x-xl)
   sanity_check
   sub_d2x_xl $2
   ;;
--d2x-xl-update)
   sanity_check
   sub_d2x_xl_update $2
   ;;
--download-only)
   sanity_check
   sub_stuff_download $2
   ;;
--backup-download)
   sub_create_backup
   ;;
--restore-backup)
   sub_restore_backup
   ;;
*)
   echo -e "\n${EYELLOW}D2X-XL: full (cd-iso-demo) install and update script -- version 1.0\n \
   ${EGREY}\n Usage: ${EWHITE}$0 --cd-d1 [/path/to/d1cd_track1data.{iso|cue}]${EGREY}\n\t\tbegin the installation from DESCENT cd or from an user iso's or cue-bin \
   \n\t\tand then continues with the next cd \
   ${EWHITE}\n\n\t$0 --cd-d2 [/path/to/d2cd_track1data.{iso|cue}]${EGREY}\n\t\tbegin the installation from DESCENT II cd or from an user iso's or cue-bin \
   \n\t\tand then continues with the next cd \
   ${EWHITE}\n\n\t$0 --cd-d2-vertigo [/path/to/d2vertigocd_track1data.{iso|cue}]${EGREY}\n\t\tbegin the installation from D2 VERTIGO cd or from an user iso's or cue-bin \
   \n\t\tand then continues with the d2x-xl install \
   ${EWHITE}\n\n\t$0 --d2x-xl <base|full>${EGREY}\n\t\td2x-xl install in basic mode or with all addons \
   ${EWHITE}\n\n\t$0 --d2x-xl-update [rebuild]${EGREY}\n\t\tperform updates or simply rebuild the d2x-xl executable\
   ${EWHITE}\n\n\t$0 <--download-only [base|full|demo]|--backup-download|--restore-backup> \
   ${EGREY}\n\t\tThe first option is self-explanatory\n\t\tThe second option is to use after installation \
   \n\t\t  for store all download data in a convenient archive.\n\t\tThe last option uses the file created previously \
   \n\t\t  to avoid downloading for future installation of D2X-XL.\n\t\tThe archive should be in the same directory as the script $0 \n"
   ;;
esac

$ERESET

exit 0
