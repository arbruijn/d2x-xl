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


###six d2x-xl full (cd-iso-demo) install and update script V2.0

D2XURL="http://www.descent2.de"
WGETD2XURL="${D2XURL}/files"
SVND2XURL="https://d2x-xl.svn.sourceforge.net/svnroot/d2x-xl/trunk"
WGETCMD="wget -c"
USRPREFIX="/usr/local"
D2XROOT="${USRPREFIX}/games/descent2"
CDLOOP="/tmp/loopmount"
D2XSTUFF="D2X-XL-stuff"

# color for echo
EYELLOW='\E[1;33;40m'
EGREEN='\E[1;32;40m'
ERED='\E[31;40m'
EWHITE='\E[1;37;40m'
EGREY='\E[0;37;40m'
ERESET="tput sgr0"

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
    *)
	echo -e "${ERED}\n\tbuild failed! probably the problem will be fixed in the next version, \
	\n\tjust wait few days and try again with '$0 --d2x-xl-update'\n"
	break
	;;
    esac
}

wget_addons_base()
{
    # d2x-xl source for linux and d2x-xl-data + bin for windows (base/pure install)
    cd ${D2XROOT}/${D2XSTUFF}/downloads/d2x-xl-data
    echo > ../../status/wgetlog/base
    # found installed version
    INUSEVER="$(ls -1v d2x-xl-src-* 2> /dev/null | grep -iv current | tail -n1 | cut -d"-" -f4 | cut -d"." -f -3)"
    if [ "0$INUSEVER" = "0" ] ; then
	echo "${ARRAYVER[2]}" > ../../status/inusever
    else
	[ "$INUSEVER" != "${ARRAYVER[2]}" -o ! -f ../../status/inusever ] && echo "$INUSEVER" > ../../status/inusever
    fi
    # download current version
    if [ "0${ARRAYVER[2]}" != "0" ] ; then
	script -qac "$WGETCMD ${WGETD2XURL}/d2x-xl-win-${ARRAYVER[2]}.exe" ../../status/wgetlog/base
	script -qac "$WGETCMD ${WGETD2XURL}/d2x-xl-src-${ARRAYVER[2]}.7z" ../../status/wgetlog/base
	rm -f $(find . -type l)
	ln -s d2x-xl-win-${ARRAYVER[2]}.exe d2x-xl-win-current.exe
	ln -s d2x-xl-src-${ARRAYVER[2]}.7z d2x-xl-src-current.7z
    else
	echo -e "${ERED}\n\tCurrent version unknown, can not download D2X-XL source and data\n"
	sleep 2
    fi
}

wget_addons_allgoodies()
{
    # hires models
    cd ${D2XROOT}/${D2XSTUFF}/downloads/models
    echo > ../../status/wgetlog/models
    MODELS="gun-laser.7z	gun-superlaser.7z
	gun-spreadfire.7z	gun-helix.7z
	gun-plasma.7z		gun-phoenix.7z
	gun-fusion.7z		gun-omega.7z
	gun-vulcan.7z		gun-gauss.7z
	msl-concussion.7z	msl-flash.7z
	msl-homing.7z		msl-guided.7z
	msl-smart.7z		msl-mercury.7z
	msl-mega.7z		msl-eshaker.7z
	mine-smart.7z		mine-proxy.7z
	dev-afterburner.7z	dev-converter.7z
	dev-headlight.7z	dev-ammorack.7z
	dev-fullmap.7z		dev-quadlaser.7z
	dev-vulcanammo.7z	dev-slowmotion.7z
	dev-bullettime.7z	dev-keys.7z
	hostage.7z		bullet.7z
	ship-pyrogl.7z		ship-phantomxl.7z
	ship-wolf.7z"
    for RAR in $MODELS
    do
	script -qac "$WGETCMD ${WGETD2XURL}/models/$RAR" ../../status/wgetlog/models
    done
    # hires textures d2
    cd ../textures
    echo > ../../status/wgetlog/textures
    TEXTURES="D2-hires-ceilings.7z	D1-hires-ceilings.7z
	D2-hires-doors.7z		D1-hires-doors.7z
	D2-hires-fans-grates.7z		D1-hires-fans-grates.7z
	D2-hires-lava-water.7z		D1-hires-lava.7z
	D2-hires-lights.7z		D1-hires-lights.7z
	D2-hires-metal.7z		D1-hires-metal.7z
	D2-hires-rock.7z		D1-hires-rock.7z
	D2-hires-signs.7z		D1-hires-signs.7z
	D2-hires-special.7z		D1-hires-special.7z
	D2-hires-switches.7z		D1-hires-switches.7z
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
    MODS="D2-Mod-1.7z		D1-Quake-Mod.7z
	D2-Mod-2.7z		Weapon-and-Effects-Mod.7z"
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
    $ERESET
    cd ${D2XROOT}/${D2XSTUFF}/downloads/d2demo
    echo > ../../status/wgetlog/demo
    script -qac "$WGETCMD ${WGETD2XURL}/gamedata/D2-demodata.7z" ../../status/wgetlog/demo
}

svn_src_download()
{
    echo -e "${EWHITE}\n\tDownload d2x-xl SVN version\n"
    $ERESET
    sleep 2
    cd ${D2XROOT}/${D2XSTUFF}/SVNbuild
    rm -fR trunk
    svn co $SVND2XURL
    cd -
}

download_only()
{
    $ERESET
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
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Create D2X-XL-stuff Backup >>>\n"
    $ERESET
    sleep 2
    cd ${D2XROOT}
    tar -vczf $CURRDIR/${D2XSTUFF}.tar.gz --exclude=*build --exclude=status ${D2XSTUFF}
    echo -e "${EWHITE}\n\tBackup saved in -> $CURRDIR/${D2XSTUFF}.tar.gz\n"
}

sub_restore_backup()
{
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Restore D2X-XL-stuff Backup >>>"
    $ERESET
    sleep 2
    if [ -f $CURRDIR/${D2XSTUFF}.tar.gz ] ; then
	mkdir -p ${D2XROOT}
	tar -vxzf $CURRDIR/${D2XSTUFF}.tar.gz -C ${D2XROOT}
    else
	echo -e "${ERED}\n\t$CURRDIR/${D2XSTUFF}.tar.gz file not found!\n"
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
	    break
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
	echo -e "${EWHITE}\n\tInstall or Update d2x-xl base\n"
	$ERESET
	sleep 2
	rm -fR build
	mkdir -p build
	[ "$1" != "rebuild" ] && unrar x -y downloads/d2x-xl-data/d2x-xl-win-current.exe ${D2XROOT}/
	7z x -bd -y -o./build/ downloads/d2x-xl-data/d2x-xl-src-current.7z
	cd build
	7z x -bd -y ./d2x-xl-makefiles.7z
	chmod 0755 ./autogen.sh ./configure
    fi
}

install_addons_allgoodies()
{
    cd ${D2XROOT}/${D2XSTUFF}/downloads
    echo -e "${EWHITE}\n\tInstall or Update d2x-xl allgoodies"
    $ERESET
    sleep 2
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
    sed -i -e "sY.*\\\Y\t\t\t\*BITMAP \"Yg" *.ase > /dev/null 2>&1
    cd - > /dev/null 2>&1
}

manage_iso()
{
    REQCD="$1"
    OWNISO="$2"
    if [ "0$OWNISO" = "0" ] ; then
	check_iso $REQCD
    else
	if [ -f "$OWNISO" ] ; then
	    case $OWNISO in
	    *.iso)
		echo -e "${EWHITE}\n\tCopy .iso in the ${D2XSTUFF}/ISO path"
		cp -afv $OWNISO ${D2XROOT}/${D2XSTUFF}/ISO/$REQCD.iso
		touch ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid
		;;
	    *.cue)
		create_iso_from_bin $OWNISO $REQCD
		;;
	    *)
		echo -e "${ERED}\n\tCD-image type not supported"
		;;
	    esac
	else
	    echo -e "${ERED}\n\tThe file $OWNISO not exist!!"
	fi
    fi
}

check_iso()
{
    REQCD="$1"
    echo -e "${EWHITE}\n\tInstall the game directly from the CD is not recommended, because the Mixed Mode CDs \
    \n\tare often the cause of problems. To avoid any kind of problem with mount or emulation (DOSBox) \
    \n\tand the format of the data contained in the CD, D2X-XL-install automatically generate the ISO \
    \n\tfrom which we will run the setup, in a session DOSBox"
    if [ -s ${D2XROOT}/${D2XSTUFF}/ISO/$REQCD.iso ] ; then
	echo -e "${EWHITE}\n\t$REQCD.iso found in the path ${D2XSTUFF}/ISO, try installing from that? -> default yes"
	read_prompt yes yes no
	[ "$ANSWER" = "yes" ] && touch ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid
    fi
    if [ "$ANSWER" = "no" -o ! -s ${D2XROOT}/${D2XSTUFF}/ISO/$REQCD.iso ] ; then
	echo -e "${EWHITE}\n\tcreation of the ${D2XSTUFF}/ISO/$REQCD.iso Track 1 data from CD\n"
	$ERESET
	create_iso_from_cd $REQCD
    fi
}

create_iso_from_cd()
{
    REQCD="$1"
    # find the cdrom in the computer
    CDLIST="$(cdrecord --devices -s | egrep -v -i "wodim:|----" | cut -d"'" -f2)" # |virt
    for CD in $CDLIST
    do
	echo -e "${EWHITE}\tI try with the drive -> $(cdrecord --devices -s dev=$CD | egrep -v -i "wodim:|----" | cut -d"'" -f4,6)" # |virt
	# find the label of current cdrom
	CDLABEL=$(cd-info --no-device-info -T -C $CD 2> /dev/null | tr -d "[:blank:]" | egrep -i "volume:" | cut -d":" -f2)
	# check if the current cdrom is that required
	if [ "$CDLABEL" = "$REQCD" ] ; then
	    echo -e "${EWHITE}\t$REQCD CD found!"
	    TRACKRANGE=($(cd-info --no-device-info -C $CD | tr -s "[:blank:]" " " | egrep -i " 1:| 2:" | cut -d" " -f4))
	    readcd dev=$CD f=${D2XROOT}/${D2XSTUFF}/ISO/${CDLABEL}.iso sectors=${TRACKRANGE[0]}-${TRACKRANGE[1]}
	    if [ "0$?" = "00" ] ; then
		touch ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid
		break
	    else
		echo -e "${ERED}\tCreation of the ${D2XSTUFF}/ISO/$REQCD.iso FAILED!"
	    fi
	    echo
	else
	    echo -e "${ERED}\t$REQCD CD not found in this drive!"
	fi
    done
}

create_iso_from_bin()
{
    REQCD="$2"
    LINEPATH="$(dirname $1)"
    LINEFILE="$(basename $1)"
    cd $LINEPATH
    LINEFILEBIN="$(grep -i "file" $LINEFILE | cut -d'"' -f2)"
    REALFILEBIN="$(ls $(echo $LINEFILE | cut -d"." -f1)* 2> /dev/null | grep -i bin)"
    if [ "0$REALFILEBIN" = "0" ] ; then
	echo -e "${ERED}\n\t${LINEPATH}/${LINEFILE}.bin not found!\n\t.cue and .bin files must be in the same directory"
    else
	$ERESET
	[ "$REALFILEBIN" != "$LINEFILEBIN" ] && mv "$REALFILEBIN" "$LINEFILEBIN"
	bchunk -w $LINEFILEBIN $LINEFILE $REQCD
	if [ "0$?" = "00" ] ; then
	    touch ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid
	else
	    echo -e "${ERED}\tCreation of the ${D2XSTUFF}/ISO/$REQCD.iso FAILED!"
	fi
	mv -f ${REQCD}01.iso ${D2XROOT}/${D2XSTUFF}/ISO/$REQCD.iso
	cd - > /dev/null 2>&1
    fi
}

dosbox_install()
{
    REQCD="$1"
    cd ${D2XROOT}/${D2XSTUFF}/ISO
    case $REQCD in
    DESCENT)
	echo -e "${EWHITE}\n\tDESCENT setup with DOSBox: Leave all the setup options to default values (just hit enter) \
	\n\texcept for the question '${EGREEN}shareware version of descent${EWHITE}' -> answer >${EGREEN}don't copy data file${EWHITE}< \
	\n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
	chmod o+w ${USRPREFIX}/games
	;;
    DESCENT_II)
	echo -e "${EWHITE}\n\tDESCENT_II setup with DOSBox: Leave all the setup options to default values (just hit enter) \
	\n\texcept for the question '${EGREEN}do you whish to continue${EWHITE}' -> answer >${EGREEN}yes${EWHITE}< \
	\n\tand make sure that the '${EGREEN}installation choices${EWHITE}' is >${EGREEN}crazy${EWHITE}< \
	\n\tand resolution of movies is >${EGREEN}high${EWHITE}< \
	\n\tFor the question '${EGREEN}S3 version of D2${EWHITE}' -> answer >${EGREEN}no${EWHITE}< \
	\n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
	chmod -R o+w ${D2XROOT}
	;;
    D2_VERTIGO)
	echo -e "${EWHITE}\n\tD2_VERTIGO setup with DOSBox: Leave all the setup options to default values (just hit enter) \
	\n\tand don't forget to select >${EGREEN}Exit${EWHITE}< at the end of installation process!"
	cp -af ../status/backup4d2_vertigo/* ../../
	chmod -R o+w ${D2XROOT}
	;;
    esac
    $ERESET
    sleep 3
    mount -o loop $REQCD.iso $CDLOOP > /dev/null 2>&1
    su $GUESSEDUSER -c "dosbox -c \"mount c ${USRPREFIX}\" -c \"mount d $CDLOOP -t cdrom -label $REQCD\" -c \"d:\" -c \"install.exe\" -c \"exit\"" > /dev/null 2>&1
    case $REQCD in
    DESCENT)
	# 'level' is already in 'dlotw' and 'newlevel' is already in 'descent'
	cp -af ${CDLOOP}/dlotw/* ${USRPREFIX}/games/DESCENT/
	chmod o-w ${USRPREFIX}
	;;
    DESCENT_II)
	mkdir -p ../status/backup4d2_vertigo
	mv ../../DESCENT2.EXE ../status/backup4d2_vertigo/
	chmod -R o-w ${D2XROOT}
	;;
    D2_VERTIGO)
	chmod -R o-w ${D2XROOT}
	;;
    esac
    touch ../status/$REQCD-dosbox.pid
    umount $CDLOOP
}

install_d2_demo()
{
    cd ${D2XROOT}/${D2XSTUFF}/downloads/d2demo
    7z x -y -o../../../data/ D2-demodata.7z
}

fix_perm_fix_lowercase()
{
    cd ${D2XROOT}
    echo -e "${EWHITE}\n\tSet the owner, permissions and lowercase"
    chown -R root:root $(ls -A) 2> /dev/null
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
    mv -f *.{cfg,CFG} *.{ini,INI} ${D2XROOT}/config/ > /dev/null 2>&1
    mv -n *.{dem,DEM} ${D2XROOT}/demos/ > /dev/null 2>&1
    mv -f *.{hog,HOG} *.{msn,MSN} *.{ham,HAM} *.{pig,PIG} *.{s??,S??} ${D2XROOT}/data/ > /dev/null 2>&1
    mv -n *.{plr,PLR} *.{plx,PLX} ${D2XROOT}/profiles/ > /dev/null 2>&1
    mv -n *.{mvl,MVL} ${D2XROOT}/movies/ > /dev/null 2>&1
}

build_install_d2x_xl()
{
    if [ "$1" = "svn" ] ; then
	cd ${D2XROOT}/${D2XSTUFF}/SVNbuild/trunk
    else
	cd ${D2XROOT}/${D2XSTUFF}/build
    fi
    if [ -f ./autogen.sh ] ; then
	echo -e "${EWHITE}\n\tBuild and Install d2x-xl executable\n"
	$ERESET
	sleep 3
	sh ./autogen.sh
	./configure --enable-release=yes --enable-debug=no
	J=$(( $(egrep "^processor" /proc/cpuinfo | tail -n1 | cut -d":" -f2) + 2 ))
	[ "$J" -le "2" ] && J="1"
	while true
	do
	    make -j $J 2> ${D2XROOT}/${D2XSTUFF}/status/d2x-xl-make.log
	    if [ -f ./d2x-xl ] ; then
		[ -f ${D2XROOT}/d2x-xl ] && mv ${D2XROOT}/d2x-xl ${D2XROOT}/d2x-xl-${INUSEVER}.backup
		cp -af d2x-xl ${D2XROOT}/
		touch ${D2XROOT}/${D2XSTUFF}/status/build.pid
		break
	    else
		grep -qsi "no such file or directory" ${D2XROOT}/${D2XSTUFF}/status/d2x-xl-make.log
		if [ "0$?" = "00" ] ; then
		    echo -e "${EWHITE}\n\tThere are missing file in the d2x-xl-src archive..\n\tdo you want download missing file from the SVN version? -> default yes"
		    read_prompt yes yes no
		    case $ANSWER in
		    yes)
			svn_src_download
			find . -depth -type f -name "*.cpp" -or -name "*.h" -or -name "*.a" > /tmp/buildlist
			cd ${D2XROOT}/${D2XSTUFF}/SVNbuild/trunk
			find . -depth -type f -name "*.cpp" -or -name "*.h" -or -name "*.a" > /tmp/trunklist
			MISSFILE="$(cat /tmp/trunklist /tmp/buildlist | sort | uniq -u)"
			for I in $MISSFILE
			do
			    cp -af $I ../../build/$I
			done
			cd ${D2XROOT}/${D2XSTUFF}/build
			;;
		    no)
			echo -e "${ERED}\n\tBuild failed! probably the problem will be fixed in the next version, \
			\n\tjust wait few days and try again with '$0 --d2x-xl-update'\n"
			$ERESET
 			break
			;;
		    esac
		fi
		# find files to patch if exist
		PATCHF="$(grep -i "undefined reference to" ${D2XROOT}/${D2XSTUFF}/status/d2x-xl-make.log | head -n1 | cut -d":" -f1)"
		if [ "0$PATCHF" != "0" ] ; then
		    patch_d2x_xl $PATCHF
		fi
	    fi
	done
    else
	echo -e "${ERED}\n\tNo src to build!\n"
    fi
}

manage_install()
{
    REQCD="$1"
    OWNISO="$2"
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< $1 Install >>>"
    while true
    do
	manage_iso $REQCD $OWNISO
	if [ -f ${D2XROOT}/${D2XSTUFF}/status/$REQCD.pid ] ; then
	    dosbox_install $REQCD
	    move_to_newpath $REQCD
	    case $REQCD in
	    DESCENT)
		next_choice DESCENT_II
		;;
	    DESCENT_II)
		next_choice D2_VERTIGO
		;;
	    D2_VERTIGO)
		next_choice D2X_XL
		;;
	    esac
	else
	    echo -e "${EWHITE}\n\tDo you want to retry? -> default yes"
	    read_prompt yes yes no
	    if [ "$ANSWER" = "yes" ] ; then
		next_choice $REQCD retry
	    else
		case $REQCD in
		DESCENT)
		    next_choice DESCENT_II
		    ;;
		DESCENT_II)
		    next_choice D2_VERTIGO
		    ;;
		D2_VERTIGO)
		    next_choice D2X_XL
		    ;;
		esac
	    fi
	fi
    done
}

next_choice()
{
    REQCD="$1"
    OPTION="$2"
    if [ "$OPTION" = "retry" ] ; then
	check_prompt yes yes no
    else
	if [ "$REQCD" = "D2X_XL" ] ; then
	    check_prompt no yes no
	else
	    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< $REQCD Install >>>\n\n\tDo you want to install the '$REQCD' CD ? -> default yes"
	    read_prompt yes yes no
	fi
    fi
    case $ANSWER in
    yes)
	OWNISO=""
	echo -e "${EWHITE}\n\tyou can also specify a .cue or .iso previously created (not for those already in ${D2XSTUFF}/ISO) -> default cdrom"
	read_prompt cdrom cdrom iso cue
	case $ANSWER in
	iso)
	    echo -e ${EGREEN}
	    read -p " [/path/to/$REQCD-cd-image.iso] > " OWNISO
	    ;;
	cue)
	    echo -e ${EGREEN}
	    read -p " [/path/to/$REQCD-cd-image.cue] > " OWNISO
	    ;;
	esac
	;;
    no)
	echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< D2X-XL Install >>>\n\n\tDo you want to continue with the installation of D2X-XL ? -> default yes"
	read_prompt yes yes no
	case $ANSWER in
	yes)
	    fix_perm_fix_lowercase
	    sub_d2x_xl
	    break
	    ;;
	no)
	    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Finalize Installation >>>"
	    fix_perm_fix_lowercase
	    break
	    ;;
	esac
	;;
    esac
}

sub_d2x_xl()
{
    if [ "0$1" = "0" ] ; then
	echo -e "${EWHITE}\n\tWhich type of d2x-xl installation you want? -> default full"
	read_prompt full base full
    else
	check_prompt $1 base full
    fi
    echo -e "$ANSWER\n" > ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype
    UPDATECK="no"
    echo -e "${EWHITE}\n\tDownload $ANSWER necessary files...\n"
    sleep 2
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
	echo -e "${EWHITE}\n\tDo you want download and build the SVN version? -> default no"
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
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Update >>>"
    case $1 in
    curr-ver)
	echo -e "${EWHITE}\n\tD2X-XL Current Version\t--> ${ARRAYVER[2]}\n"
	;;
    rebuild)
	UPDATECK="no"
	install_addons_base rebuild
	build_install_d2x_xl
	echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< End >>>\n\n\tOk! go to desktop menu and launch the game!\n\tor.., \
	\n\tOk! from an user shell, run ${D2XROOT}/d2x-xl to launch the game!\n"
	;;
    svn-rebuild)
	svn_src_download
	build_install_d2x_xl svn
	echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< End >>>\n\n\tOk! go to desktop menu and launch the game!\n\tor.., \
	\n\tOk! from an user shell, run ${D2XROOT}/d2x-xl to launch the game!\n"
	;;
    "")
	D2XINSTTYPE="$(cat ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype)"
	[ "0$D2XINSTTYPE" = "0" ] && D2XINSTTYPE="full"
	UPDATECK="yes"
	echo -e "${EWHITE}\n\tCheck and download updates...\n"
	sleep 2
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
		echo -e "${EWHITE}\n\tDo you want download and build the SVN version? -> default no"
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
	;;
    esac
}

sub_stuff_download()
{
    if [ "0$1" = "0" ] ; then
	echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Download Only >>>\n\n\tWhich type of d2x-xl download you want? -> default full"
	read_prompt full base full demo
    else
	check_prompt $1 base full demo
    fi
    echo
    download_only $ANSWER
}

create_menu_entry()
{
    if [ ! -f /usr/share/applications/d2x-xl.desktop ] ; then
	echo -e "[Desktop Entry]\nEncoding=UTF-8\nExec=${D2XROOT}/d2x-xl\nIcon=/usr/share/pixmaps/d2x-xl-0.png\nType=Application \
	\nTerminal=false\nName=D2X-XL\nGenericName=Six degrees of freedom FPS\nStartupNotify=true \
	\nCategories=Game;ActionGame;\n" > /usr/share/applications/d2x-xl.desktop
	convert ${D2XROOT}/d2x-xl.ico /usr/share/pixmaps/d2x-xl.png
	chmod 644 /usr/share/applications/d2x-xl.desktop /usr/share/pixmaps/d2x-xl*.png
    fi
}

home_user_config()
{
    # for linux user
    LU=""
    PS3="next user number: > "
    cd ${D2XROOT}/${D2XSTUFF}/status/users/
    echo -e ${EGREEN}
    select LU in $HUSERS Done
    do
	[ "$LU" = "Done" ] && break
	if [ ! -f $LU ] ; then
	    mkdir -p /home/$LU/.d2x-xl
	    cp -af ${D2XROOT}/{config,profiles,savegames,screenshots,demos,cache} /home/$LU/.d2x-xl/
	    dos2unix -b -U /home/$LU/.d2x-xl/config/* > /dev/null 2>&1
	    echo -e "\n\n-sound22k\n-noredundancy\n-pps 10\n-use_d1sounds 1\n" >> /home/$LU/.d2x-xl/config/d2x-default.ini
	    chown -R $LU:users /home/$LU/.d2x-xl
	    touch $LU
	    echo -e "${EWHITE}\n\tUser "${EGREY}$LU${EWHITE}" is done.\n\tThe default configuration file is -> /home/$LU/.d2x-xl/config/d2x-default.ini"
	else
	    echo -e "${EWHITE}\n\tUsers already configured:${EGREY} $(ls | tr "\n" "," | sed -e "s/,/, /g")"
	fi
	echo -e ${EGREEN}
    done
    PS3="> "
    cd - > /dev/null 2>&1
}

finalize_installation()
{
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Finalize Installation >>>"
    fix_perm_fix_lowercase
    do_clean
    if [ "$1" = "onlyclean" ] ; then
	echo -e "\n\tIn the next time, to retry do not forget to use '$0 --d2x-xl-update'\n"
    else
	if [ "$1" = "update" ] ; then
	    echo -e "${EWHITE}\n\tlist of users already configured: \
	    \n${EGREY}$(ls ${D2XROOT}/${D2XSTUFF}/status/users | tr "\n" "," | sed -e "s/,/, /g")\n\t${EWHITE}do you want add more user? -> default no"
	    read_prompt no yes no
	    [ "$ANSWER" = "yes" ] && home_user_config
	else
	    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Configuration of the Home User >>>\n\n\tD2x-xl in linux keeps profiles, saved games, screenshots, etc.. of different users \
	    \n\tin their home directory.\n\tWhich users must be configured?"
	    home_user_config
	fi
	create_menu_entry
	echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< End >>>\n\n\tOk! go to desktop menu and launch the game!\n\tor.., \
	\n\tOk! from an user shell, run ${D2XROOT}/d2x-xl to launch the game!\n"
    fi
}

distro_dep()
{
    for TRY in 1 2
    do
	case $DISTRO in
	arch)
	    MISSPKG=($(pacman -Q $DEPPKG 2>&1 | grep -i "not found" | cut -d'"' -f2 ))
	    ;;
	ubuntu)
	    MISSPKG=($(dpkg -l $DEPPKG 2>&1 | egrep -i "^no packages found" | cut -d" " -f5 | tr -d "." ) $(dpkg -l $DEPPKG | egrep -v "^*i |\=|^\|" | tr -s "[:blank:]" "\t" | cut -f2))
	    ;;
	fedora)
	    MISSPKG=($(rpm -q $DEPPKG | grep -i "not installed" | cut -d" " -f2 ))
	    ;;
	SuSE)
	    MISSPKG=($(zypper -q if $DEPPKG | grep -i "not found" | cut -d"'" -f2 ))
	    for MPKG in $DEPPKG
	    do
		zypper -q if $MPKG | egrep -qs "^Status: not installed" && MISSPKG=(${MISSPKG[*]} $MPKG)
	    done
	    ;;
	unknown)
	    echo -e "\t${EWHITE}Package dependencies\t--> ${ERED}Distro not supported...\n\n\t${EWHITE}Want to ignore the warnings about dependencies, and continue the installation,\
	    \n\tbecause you know you have already installed the required dependencies manually? -> default no"
	    read_prompt no yes no
	    if [ "$ANSWER" = "yes" ] ; then
		MISSPKG=""
	    else
		MISSPKG="$DEPPKG"
	    fi
	    break
	    ;;
	esac
	if [ "0${MISSPKG[*]}" = "0" ] ; then
	    echo -e "\t${EWHITE}Package dependencies\t--> OK"
	    break
	else
	    if [ "$TRY" = "2" ] ; then
		break
	    else
		echo -e "\t${EWHITE}Package dependencies\t--> ${ERED}Missing something needed for correct build of d2x-xl executable\n\n\t${EWHITE}Do you want install them directly, or you prefer a list to install them manually? -> default directly"
		read_prompt directly directly list
		if [ "$ANSWER" = "directly" ] ; then
		    case $DISTRO in
		    arch)
			pacman -S --needed --noconfirm ${MISSPKG[*]}
			;;
		    ubuntu)
			apt-get -y install ${MISSPKG[*]}
			;;
		    fedora)
			if [ "0$(rpm -qa "*rpmfusion-free-release*")" = "0" ] ; then
			    echo -e "\n\t${EWHITE}Add fedora repository\t--> RPM Fusion"
			    $ERESET
			    yum localinstall -y --nogpgcheck http://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-stable.noarch.rpm http://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-stable.noarch.rpm
			fi
			yum install -y ${MISSPKG[*]}
			;;
		    SuSE)
			REALMISSPKG="$(echo "${MISSPKG[*]}" | sed -e "s/bchunk//g")"
			[ "0$REALMISSPKG" != "0" ] && zypper -n in $REALMISSPKG
			echo "${MISSPKG[*]}" | grep -qs "bchunk"
			if [ "0$?" = "00" ] ; then
			    echo -e "\n\t${EWHITE}There is still some dependency package not contained in the official repositories, \
			    \n\tplease install it from this URL:\n\\n${EGREY}\thttp://packman.links2linux.org/package/bchunk\n"
			fi
			;;
		    esac
		else
		    break
		fi
	    fi
	fi
    done
}

sanity_check()
{
    echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< Sanity Check >>>\n"
    if [ "0$(id -u)" != "00" ] ; then
	echo -e "${ERED}\tThis script should be run as root!!!\n"
	$ERESET
    else
	# find current distro
	LUIDRANGE="1[0-9]{3}"
	GURANGE="1,5"
	DISTRO="unknown"
	DEPPKG="libtool, gcc, gcc-c++, autoconf, make, automake, nasm, m4, bison, \n \
	    subversion, patch, SDL-complete and SDL-complete-devel, OpenMotif or lesstif and -devel, \n \
	    glew and glew-devel and/or glew-utils, curl and curl-devel, glut and glut-devel, \n \
	    mesa and mesa-devel, unrar, p7zip, dos2unix, imagemagick, dosbox, libcdio, bchunk"
	grep -qsi "arch linux" /etc/rc.conf
	if [ "0$?" = "00" ] ; then
	    DISTRO="arch"
	    DEPPKG="libtool gcc autoconf automake make nasm m4 bison subversion patch flex pkg-config fakeroot sdl sdl_gfx sdl_image sdl_pango sdl_mixer \
		sdl_net sdl_sound sdl_ttf lesstif glew curl freeglut mesa wget unrar p7zip hd2u dosbox imagemagick cdrkit libcdio bchunk"
	else
	    grep -qsi "ubuntu" /etc/issue
	    if [ "0$?" = "00" ] ; then
		DISTRO="ubuntu"
		DEPPKG="libtool build-essential autoconf automake nasm m4 bison subversion patch libsdl-gfx1.2-4 libsdl-gfx1.2-dev libsdl-image1.2 libsdl-image1.2-dev \
		    libsdl-mixer1.2 libsdl-mixer1.2-dev libsdl-net1.2 libsdl-net1.2-dev libsdl-sound1.2 libsdl-sound1.2-dev libsdl1.2-dev libsdl1.2debian libsdl1.2debian-pulseaudio \
		    libmotif3 libmotif-dev libglew1.5-dev libglew1.5 glew-utils libcurl4-gnutls-dev curl freeglut3-dev mesa-common-dev wget unrar p7zip dos2unix imagemagick \
		    dosbox libcdio-utils bchunk"
	    else
		grep -qsi "fedora" /etc/redhat-release
		if [ "0$?" = "00" ] ; then
		    LUIDRANGE="5[0-9]{2}"
		    DISTRO="fedora"
		    DEPPKG="libtool gcc gcc-c++ autoconf automake make nasm m4 bison subversion patch SDL SDL-devel SDL_gfx SDL_gfx-devel SDL_image SDL_image-devel SDL_mixer \
			SDL_mixer-devel SDL_net SDL_net-devel SDL_Pango SDL_Pango-devel SDL_sound SDL_sound-devel SDL_ttf SDL_ttf-devel lesstif-devel lesstif glew-devel \
			glew libcurl-devel curl freeglut-devel freeglut mesa-libGL-devel mesa-libGLU-devel unrar p7zip dos2unix ImageMagick dosbox libcdio-devel libcdio bchunk wget"
		else
		    grep -qsi "suse" /etc/SuSE-release
		    if [ "0$?" = "00" ] ; then
			DISPLAY="$(echo $DISPLAY | cut -d"." -f1)"
			GURANGE="-2"
			DISTRO="SuSE"
			DEPPKG="libtool gcc gcc-c++ autoconf make automake nasm m4 bison subversion patch libSDL_Pango-devel libSDL_Pango1 libSDL_gfx-devel libSDL_gfx13 libSDL_image-1_2-0 \
			    libSDL_image-devel libSDL_mixer-1_2-0 libSDL_mixer-devel libSDL_net-1_2-0 libSDL_net-devel libSDL_sound-1_0-1 libSDL_sound-devel libSDL_ttf-2_0-0 libSDL_ttf-devel \
			    openmotif openmotif-devel glew glew-devel curl libcurl-devel freeglut freeglut-devel mesa mesa-devel wget unrar p7zip dos2unix imagemagick dosbox libcdio-utils \
			    libSDL-1_2-0 libSDL-devel bchunk"
		    fi
		fi
	    fi
	fi
	echo -e "\t${EWHITE}Linux distro\t\t--> $DISTRO"
	# check the current user login in graphic mode, will be used by DOSBox
	HUSERS="$(getent passwd | cut -d":" -f-3 | egrep ":${LUIDRANGE}$" | cut -d":" -f1)"
	if [ "0$DISPLAY" = "0" ] ; then
	    echo -e "${ERED}\n\t$0: did not found any active X session, setup can't continue.\n"
	    [ "$DISTRO" = "SuSE" ] && echo -e "\tDo not use >${EWHITE}sudo${ERED}< to become 'root', use >${EWHITE}su - ${ERED}< instead...\n"
	    $ERESET
	else
	    GUESSEDUSER="$(who | tr -s "[:blank:]" "\t" | cut -f $GURANGE | grep "$DISPLAY" | cut -f 1 | uniq)"
	    if [ "0$GUESSEDUSER" = "0" ] ; then
		echo -e "${ERED}\n\t!! ATTENTION !!${EWHITE}\n\t$0: did not found the real owner of the X session, to run dosbox. \
		\n\tplease type the name of X login user, or the user that owns the X session -> default user"
		read_prompt user $HUSERS
		GUESSEDUSER="$ANSWER"
	    else
		echo -e "\t${EWHITE}X login username\t--> $GUESSEDUSER"
	    fi
	fi
	# find current vesion and test internet connection
	ARRAYVER=($(wget -O - -q ${D2XURL}/d2x.html | tr -s "<>" "\n" | grep -i "current version"))
	if [ "0${ARRAYVER[2]}" = "0" ] ; then
	    echo -e "${ERED}\n\t!! ATTENTION !!\n\tNO INTERNET CONNECTION!! if you plan to do a new D2X-XL installation, \
	    \n\tmake sure you have a backup a previous istallation...\n"
	else
	    echo -e "\t${EWHITE}Internet connection\t--> OK"
	fi
	# check free space
	mkdir -p ${D2XROOT} $CDLOOP
	SIZES=($(stat -L -f -c "%a %S" ${D2XROOT}))
	FREES=$((${SIZES[0]}*${SIZES[1]}))
	FREESMB=$(($FREES/1024/1024))
	if [ $FREES -lt 5700000000 -a ! -f ${D2XROOT}/${D2XSTUFF}/status/d2xinsttype ] ; then
	    echo -e "${ERED}\n\t!! ATTENTION !!\n\tif you plan to do a >full< install, \
	    \n\tyou need at least >5600< MB of free space\n\tand now you have $FREESMB MB free on ${USRPREFIX}/games\n"
	else
	    echo -e "\t${EWHITE}Free space nedeed\t--> OK"
	fi
	# check dependencies
	distro_dep
	if [ "0${MISSPKG[*]}" != "0" ] ; then
	    echo -e "${ERED}\n\tMissing dependencies:${EWHITE} ${MISSPKG[*]}\n\n\tInstall the missing packages and try again...\n"
	    $ERESET
	else
	    which 7z > /dev/null 2>&1
	    if [ "0$?" != "00" ] ; then
		which 7zr > /dev/null 2>&1
		if [ "0$?" = "00" ] ; then
		    ln -s $(which 7zr) /usr/bin/7z
		else
		    ln -s $(which 7za) /usr/bin/7z
		fi
	    fi
	    # create directory structure
	    cd ${D2XROOT}
	    mkdir -p cache movies savegames screenshots mods ${D2XSTUFF}/{build,SVNbuild,ISO} ${D2XSTUFF}/status/{users,wgetlog} \
	    ${D2XSTUFF}/downloads/{d2demo,d2x-xl-data,levels,menu-background,models,mods,sounds,textures}
	    [ -h ../d2x-xl ] || ln -s ./descent2 ../d2x-xl 2> /dev/null
	    [ "$1" = "sub_d2x_xl" ] && echo -e "\n${EYELLOW}D2X-XL-install::${EWHITE}  <<< D2X-XL Install >>>"
	    $1 $2 $3
	fi
    fi
}

do_clean()
{
    cd ${D2XROOT}
    INUSEVER="$(cat ${D2XSTUFF}/status/inusever)"
    # delete unnecessary files
    [ -f ${D2XSTUFF}/status/d2_vertigo.pid ] && rm -fR descent2.exe ${D2XSTUFF}/status/backup4d2_vertigo
    rm -fR ../DESCENT d2.bat descent.cfg eregcard.exe hmi*.386 setup.exe ${D2XSTUFF}/status/wgetlog/*
    grep -qs 'd2x-xl-src-' /tmp/d2x-update.log
    if [ "0$?" = "00" -o "$UPDATECK" = "no" -a -f ${D2XSTUFF}/status/build.pid ] ; then
	rm -fR ${D2XSTUFF}/build/* ${D2XSTUFF}/SVNbuild/trunk/*
	rm -f $(ls -1 d2x-xl-*.backup 2> /dev/null | egrep -v "${ARRAYVER[2]}|$INUSEVER")
	rm -f $(ls -1 ${D2XSTUFF}/downloads/d2x-xl-data/d2x-xl-*.{exe,7z} | egrep -v "${ARRAYVER[2]}|$INUSEVER|current")
    fi
}

###-MAIN-###-MAIN-###-MAIN-###

CURRDIR="$(pwd)"
export LC_MESSAGES="C"

case $1 in
--cd-d1)
    sanity_check manage_install DESCENT $2
    ;;
--cd-d2)
    sanity_check manage_install DESCENT_II $2
    ;;
--cd-d2-vertigo)
    sanity_check manage_install D2_VERTIGO $2
    ;;
--d2x-xl)
    sanity_check sub_d2x_xl $2
    ;;
--d2x-xl-update)
    sanity_check sub_d2x_xl_update $2
    ;;
--download-only)
    sanity_check sub_stuff_download $2
    ;;
--backup-download)
    sub_create_backup
    ;;
--restore-backup)
    sub_restore_backup
    ;;
*)
    echo -e "\n${EYELLOW}D2X-XL: full (cd-iso-demo) install and update script -- version 2.0\n \
    ${EGREY}\n Usage: ${EWHITE}$0 --cd-d1 [/path/to/d1-cd.{iso|cue}]${EGREY}\n\t\tbegin the installation from DESCENT cd or from an user iso's or cue-bin \
    \n\t\tand then continues with the next cd \
    ${EWHITE}\n\n\t$0 --cd-d2 [/path/to/d2-cd.{iso|cue}]${EGREY}\n\t\tbegin the installation from DESCENT II cd or from an user iso's or cue-bin \
    \n\t\tand then continues with the next cd \
    ${EWHITE}\n\n\t$0 --cd-d2-vertigo [/path/to/d2vertigo-cd.{iso|cue}]${EGREY}\n\t\tbegin the installation from D2 VERTIGO cd or from an user iso's or cue-bin \
    \n\t\tand then continues with the d2x-xl install \
    ${EWHITE}\n\n\t$0 --d2x-xl <base|full>${EGREY}\n\t\td2x-xl install in basic mode or with all addons \
    ${EWHITE}\n\n\t$0 --d2x-xl-update [rebuild|svn-rebuild|curr-ver]${EGREY}\n\t\tperform updates or simply rebuild the d2x-xl executable, also via SVN\
    \n\t\tor show the current version on $D2XURL \
    ${EWHITE}\n\n\t$0 <--download-only [base|full|demo]|--backup-d2x-xl-stuff|--restore-backup> \
    ${EGREY}\n\t\tThe first option is self-explanatory\n\t\tThe second option is to use after installation \
    \n\t\t  for store all download data in a convenient archive.\n\t\tThe last option uses the file created previously \
    \n\t\t  to avoid downloading for future installation of D2X-XL.\n\t\tThe archive should be in the same directory as the script $0 \n"
    ;;
esac

$ERESET

exit 0

