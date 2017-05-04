#!/bin/bash
# KudKernel build script
# by krasCGQ @ xda-developers
#
# Script version: 2.2
# Last updated  : March 11, 2017 20:10 (UTC+8)
#
# General Limitation:
# If script is aborted when work environment is being cleaned, sometimes it might
# mistakenly said that an interrupted process was detected. At this point, say
# 'n', so script will re-clean the environment.

# Colorize text output to distinguish differences between generated compiler
# messages and script messages.
blu=$(tput setaf 4); # Blue  - General information messages
grn=$(tput setaf 2); # Green - For successful processes
red=$(tput setaf 1); # Red   - For failed processes
rst=$(tput sgr0);    # Reset colors so it won't colorize the whole text

# Clear command logs and outputs.
clear;

echo "${blu}===============================================================================${rst}";
echo "${blu}|              KudKernel Build Script - krasCGQ @ xda-developers              |${rst}";
echo "${blu}===============================================================================${rst}";
sleep 1;

# Start counting build duration here.
SECONDS=0;

# These defined commands will stop counting build duration at this point.
time_dur() {
	duration=$SECONDS;
	echo "${blu}[INFO]${rst}  All processes took $(( $duration / 60 )) minute(s) and $(( $duration % 60 )) second(s).";
	sleep 1;
}

# Unset all environment variables after script executed or aborted.
unset_var() {
	for i in blu grn red rst SECONDS duration LC_ALL ARCH SUBARCH CROSS_COMPILE USE_CCACHE CONFIGDIR KERNEL DTB DTB32 DTB64 ANYKERNEL SIGNAPK KBUILD_BUILD_VERSION KBUILD_BUILD_USER KBUILD_BUILD_HOST T EV v DEFCONFIG DATE ZIP; do
		unset $i;
	done;
}

# If script is aborted at user's request. Exit it.
trap '{
	echo " ";
	echo "${blu}[INFO]${rst}  Script aborted at user request. Exiting...";
	sleep 1;
	time_dur && unset_var;
	exit -1;
}' INT;

echo "${blu}[INFO]${rst}  Setting up environment variables...";
sleep 1;

# Environment variables for kernel building
# Toolchain used on KudKernel building could be downloaded via git with this command below via Terminal:
# git clone https://android-git.linaro.org/git/platform/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-6.3-linaro.git
export LC_ALL=en_US.UTF-8;
export ARCH=arm64;
export SUBARCH=arm64;
export CROSS_COMPILE=../../toolchains/linaro/aarch64-linux-android-6.3/bin/aarch64-linux-android-;
export USE_CCACHE=1;
CONFIGDIR=arch/arm64/configs;
KERNEL=arch/arm64/boot/Image.gz-dtb;
DTB=boot/dts/msm8939-qrd-wt88509_64.dtb;
DTB32=../../../arm/$DTB;
DTB64=./arch/arm64/$DTB;

# Environment variables for flashable zip creation
ANYKERNEL=./Extras/AnyKernel2;
SIGNAPK=./Extras/SignApk;

# This is my identity.
export KBUILD_BUILD_VERSION=0;
export KBUILD_BUILD_USER=krasCGQ;
export KBUILD_BUILD_HOST=KudProject;

# Build kernel with total threads + 1 to ensure all resources are used
T=`echo $(( $(nproc --all) + 1 ))`;

# Define build commands before the actual commands to prevent commands to be used
# twice in the script.
build() {
	# Append the desired version to the kernel.
	EV=EXTRAVERSION=-KudKernel-v$v;
	# Build the kernel now.
	make $EV -s -j$T || if [ -f "vmlinux" ] && [ ! -f "$DTB64" ]; then
		# If build fails because the required DTB is missing, symlink the DTB to arm64
		# directory instead of moving it.
		echo " ";
		echo "${blu}[INFO]${rst}  DTB symlink seems to be missing. Create a symlink...";
		ln -s $DTB32 $DTB64;
		sleep 1;
		# Later, resume the building process so the proper kernel could be compiled.
		echo "${blu}[INFO]${rst}  Resuming the interrupted process...";
		make $EV -s -j$T;
	else
		# However, if it fails for another reason, abort the script.
		# Be sure to solve all problems before continuing it.
		echo " ";
		echo "${red}[ERROR]${rst} Build error has occured. Check compiler logs for details.";
		sleep 1;
		# Stop counting, show total time elapsed and unset all environment variables.
		time_dur && unset_var;
		exit 1;
	fi;
	# If you've no problems when building kernel, congratulations!
	echo " ";
	echo "${grn}[DONE]${rst}  Build completed. Continuing to packing process...";
	sleep 1;
}

# These are the actual commands.
if [ -f "$KERNEL" ]; then
	# If Image.gz-dtb is exist, don't do anything at this point.
	# Skip to packing process, anyway.
	echo "${blu}[INFO]${rst}  Kernel is already built. Skipping to packing process...";
	v=`cat .extraversion`;
	sleep 1;
elif [ -d "include/generated" ] && [ ! -f "vmlinux" ]; then
	# Ask for user confirmation on either continuing interrupted process or restart
	# the build. The answer should be 'y' or 'n' (lower case without quotes).
	printf "${blu}[INFO]${rst}  An interrupted process is detected. Do you want to continue? [y/n] ";
	until [ "$a" == "y" ] || [ "$a" == "n" ]; do
		read a;
		if [ "$a" == "y" ]; then
			# If it's 'y', then continue the build process.
			# However, check if '.extraversion' is exist or not. If not, ask user to re-enter
			# the version number.
			if [ -f ".extraversion" ]; then
				v=`cat .extraversion`;
			else
				printf "${blu}[INFO]${rst}  .extraversion seems to missing. Please re-enter version number: ";
				read v;
			fi;
			echo "${blu}[INFO]${rst}  Continuing build process...";
			build;
		elif [ "$a" == "n" ]; then
			# If it's 'n', then restart the build process upon user's request.
			echo "${blu}[INFO]${rst}  Restarting build...";
			sleep 1;
		else
			# However, if the answer is neither 'y' nor 'n', ask the same question until
			# the correct answer is given.
			printf "${red}[ERROR]${rst} Try again. Do you want to continue? [y/n] ";
		fi;
	done;
fi;

# Check for Image.gz-dtb if it exists or not.
# This is useful so this command won't be triggered if it exists.
if [ ! -f "$KERNEL" ]; then
	# Clean environment before the next building process.
	echo "${blu}[INFO]${rst}  Cleaning environment...";
	if [ ! -d "include/generated" ]; then
		# The environment looks f*cked up, clean the whole environment.
		make -s -j$T distclean;
	else
		# Clean all compiled files.
		make -s -j$T clean;
	fi;
	# Force generate compile.h
	if [ -f "include/generated/compile.h" ]; then
		rm -f include/generated/compile.h;
	fi;
	# If old defconfig is exist, then remove it.
	if [ -f ".config.old" ]; then
		rm -f .config.old;
	fi;
	for i in "$CONFIGDIR/*.old"; do
		rm -f $i;
	done;
	# Remove .extraversion if exists.
	if [ -f ".extraversion" ]; then
		rm -f .extraversion;
	fi;

	# Ask user to enter the desired version number.
	# This will be appended to the kernel version.
	printf "${blu}[INFO]${rst}  Please enter version number: ";
	read v;
	echo $v > .extraversion;
	sleep 1;

	# Check for these defconfigs and compile kernel with the one of these defconfigs:
	# NOTE: Sorted by priority.
	# 1) .config (will skip to compiler)
	# 2) kud-ido_defconfig (KudKernel's defconfig)
	# 3) lineage_ido_defconfig (LineageOS' defconfig)
	# 4) wt88509_64-perf_defconfig (Xiaomi OpenSource's defconfig)
	# If any of them aren't detected, abort the script.
	if [ -f ".config" ]; then
		echo "${blu}[INFO]${rst}  .config detected! Skipping to compiler...";
		sleep 1;
	else
		for i in kud-ido_defconfig lineage_ido_defconfig wt88509_64-perf_defconfig; do
			if [ -f $CONFIGDIR/$i ]; then
				echo "${blu}[INFO]${rst}  Using the following defconfig: $i";
				DEFCONFIG=$i;
				break;
			fi;
		done;
		if [ ! -f "$CONFIGDIR/$DEFCONFIG" ]; then
			echo "${red}[ERROR]${rst} No matching defconfig found! Exiting...";
			sleep 1;
			# Stop counting, show total time elapsed, and unset all environment variables.
			time_dur && unset_var;
			exit 1;
		fi;
		make -s -j$T $DEFCONFIG;
		echo " ";
	fi;

	# Everything is set up. Let's compile the kernel!
	echo "${blu}[INFO]${rst}  Building kernel...";
	build;
fi;

# Generate date and time variable to be appended with zip name.
DATE=$(date +"%Y%m%d-%H%M");
ZIP=KudKernel-ido-v$v-$DATE.zip;

# Output the kernel zip name.
echo "${blu}[INFO]${rst}  Using the following zip name: $ZIP";

# Copy the image to AnyKernel2 template directory.
cp -f $KERNEL $ANYKERNEL/Image.gz-dtb;

# Enter AnyKernel2 template directory and create a flashable zip.
echo "${blu}[INFO]${rst}  Creating flashable zip...";
cd $ANYKERNEL/;
zip -qr9 $ZIP .;

# Go back to working directory.
cd ../..;

# Move the flashable zip to SignApk directory.
mv -f $ANYKERNEL/$ZIP $SIGNAPK/$ZIP;

# Enter SignApk directory and sign the flashable zip with Android test-keys.
echo "${blu}[INFO]${rst}  Signing flashable zip...";
cd $SIGNAPK;
java -jar minsignapk.jar testkey.x509.pem testkey.pk8 $ZIP $ZIP;

# Go back to working directory.
cd ../..;

# Create the output directory if it's not exist.
if [ ! -d "out" ]; then
	mkdir out;
fi;

# Move the flashable zip to the output directory.
echo "${blu}[INFO]${rst}  Moving the flashable zip to the output directory...";
mv -f $SIGNAPK/$ZIP out/$ZIP;
sleep 1;

# The whole process is complete. Clean up residuals created.
echo "${blu}[INFO]${rst}  Cleaning up residuals...";
rm -f $KERNEL $ANYKERNEL/Image.gz-dtb .extraversion;
echo "${grn}[DONE]${rst}  Process completed.";

# Stop counting, show total time elapsed, and unset all environment variables.
time_dur && unset_var;

exit 0;
