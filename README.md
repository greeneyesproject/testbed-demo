# GreenEyes European Project - Testbed application

The software provided here is a sample application easily extensible and re-usable developed within the FP-7 founded [European project GreenEyes](http://www.greeneyesproject.eu/).

## Getting started

This document will guide you step-by-step to the setup and use of the provided software. This manual has been tested on Ubuntu 14.04LTS 64bit.

If you are not familiar with linux bash, ssh, make and c++ programming you should probably stop here and come back after having attended a computer science class.

### Prerequisites

* Linux equipped PC with
  * GNU g++ compiler
  * GNU g++ cross compiler for armhf architectures (arm-linux-gnueabihf-g++)
  * OpenCV 2.4.9 - 2.4.11 libraries
  * Boost 1.57+ libraries
  * Eigen3 library
  * QT5 and QTCreator
  * Eclipse CDT
* [BeagleBone Black](http://beagleboard.org/black) equipped with
  * a USB camera or the RadiumBoard HD Camera Cape
  * a Wifi USB adapter
  * OpenCV 2.4.9 - 2.4.11 libraries
  * Boost 1.57+ libraries
  * Eigen3 library
* (Optional) [Crossbow Telosb TPR2420](http://www.memsic.com/wireless-sensor-networks/)
* (Optional) [A small USB hub](http://www.amazon.it/AmazonBasics-Ultra-Mini-Hub-porte/dp/B003M0NURK/)
* (Optional) A 1GB+ microSD card

### Getting the code

First of all download this repository or clone it in your Linux PC.
The folder structure is the following:

* testbed-demo/
  * gui/
  * vsn/
  * thirdparty/

The whole code and configuration files were written in a way that if you preserve this folder structure your life will get easier.

## Preparing the Beaglebone

The system is tested and working with the debian-wheezy release with kernel 3.8.13-bone70. You can find the vanilla image at http://beagleboard.org/latest-images.

You should now install the libeigen3 library and compile (if you have plenty of time) or cross-compile (if you are brave enough) OpenCV and Boost. If you are willing to go for the OpenCV (cross-)compilation please be aware that if you don't link OpenCV against libjpeg turbo your USB webcam will not work as expected. Plese refer to [this page](http://blog.lemoneerlabs.com/3rdParty/Darling_BBB_30fps_DRAFT.html) for further information.

Since we know how much time and effort we needed to prepare a working OS for the BeagleboneBlack, we provide you a fully working image that you simply need to flash to a 1GB+ microSD card.
The image is available without any warranty, but it can really get your life easier.

#### Flashing your microSD card
1. Download and `tar xf` the mbr image (ftp://ftp.elet.polimi.it/outgoing/Luca.Bondi/greeneyes/bbb-mbr-20150903.tar.gz) and the rootfs image (ftp://ftp.elet.polimi.it/outgoing/Luca.Bondi/greeneyes/bbb-rootfs-20150903.tar.gz)
2. Insert yout SD card and check the device path (e.g. /dev/mmcblk0)
3. Unmount all the SD card partitions
4. Flash the MBR to the SD card: `sudo dd if=bbb-mbr-XXXXYYZZ.img of=/dev/mmcblk0`
5. Synchronize the modified mbr with the system: `sync`
6. Flash the rootfs to the SD card first partition: `sudo dd if=bbb-rootfs-XXXXYYZZ.img of=/dev/mmcblk0p1 bs=16M`
7. (optional) resize the rootfs partition to fit the SD card

#### Testing the BeagleBone Black
Insert the SD card into your BeagleBone Black, plug the USB hub (the BeagleBone needs USB Hub to be connected at startup) and plug it though the miniUSB cable to your Linux PC. Once the system is up and rounning you should be able to connect to the BeagleBone via `ssh debian@192.168.7.2` (password: temppwd).

Now it's time to check your hardware. If you already have installed the RadiumBoard Camera Cape HD you should be able to `ls /dev/video0`. If you have a USB Webcam plug it and check it's correctly recognized with `dmesg` and it appears as the `/dev/video0` device. Plug your USB WiFi adapter and check it's correctly recognized and it is listed (`sudo iwconfig` as wlan0).

### Compile thirdparty libraries
 
In your Linux PC terminal `cd thirdparty/lib-host` and `make`.
If include files are missing, please add the include path to the Makefile or install your libraries to /usr/local/.

Before cross-compiling thirdparty libraries please put the cross compiled OpenCV and Boost libraries respactively into /opt/boost-arm and /opt/opencv-arm. You can find a copy of these two libraries cross-compiled for the BeagleBone Black in the rootfs image or you can download them separately from ftp://ftp.elet.polimi.it/outgoing/Luca.Bondi/greeneyes/boost-arm.tar.gz and ftp://ftp.elet.polimi.it/outgoing/Luca.Bondi/greeneyes/opencv-arm.tar.gz.

Now you are able to cross-compile thirdparty libraries: `cd thirdparty/lib-bbb` and `make`.

### VSN code: Eclipse

The VSN code is available as an Eclipse CDT project. Simply import the vsn folder as an existing project in Eclipse and you are done. Two build configurations are available: build-host and build-bbb, for the host system and the BeagleBone Black respectively.
Once you have built both the configurations the executable files are located in `vsn/build-host/greeneyes-vsn` and `vsn/build-bbb/greeneyes-vsn`.

### GUI code: QT Creator

The GUI code is available as a QT Creator projet. From within QT Creator open the greeneyes-gui.pro project file and everything should be fine. Follow the wizard to create a build configuration and use as build path a subdirectory of the `gui/` folder, e.g. `gui/build/`.
Once you compile the code the executable is available in `gui/build/greeneyes-gui`.

## Operations

Now you should have three executable files: `vsn/build-host/greeneyes-vsn`, `vsn/build-bbb/greeneyes-vsn` and `gui/build/greeneyes-gui`.

### Configuring the BeagleBone

First we need to upload the vsn software to the BeagleBone and configure the service to start the ad-hoc WiFi connection and the vsn software ad startup.
From the `vsn/` folder upload the csv software with the provided script `./script/sendToBBB.sh debian@192.168.7.2`. This will copy all the needed files into `/opt/greeneyes-vsn`.

Now ssh into the BeagleBone, modify the `/opt/greeneyes-vsn/greeneyes` startup script as needed to select the role of the BeagleBone and copy `/opt/greeneyes-vsn/greeneyes` to `/etc/init.d/greeneyes` and make it executable (`chmod +x /etc/init.d/greeneyes`). Make the service start at boot with `sudo update-rc.d greeneyes defaults`.
Start the greeneyes service and check the log file at `/opt/greeneyes-vsn/log.txt`.


To configure multiple BeagleBone repeat the procedure above for each one. The service script configures different ip address in the ad-hoc wlan for each network node.

#### Multi BeagleBone hints
* At startup the BeagleBone executes the `/opt/scripts/boot/am335x_evm.sh` script that configures the USB ethernet interface.
* The USB ethernet DHCP configuration is in `/etc/udhcp.conf`.
* Check the `/etc/network/interfaces` file for all the network configurations.
* The hostname can be set into `/etc/hostname`. Remember to change also `/etc/hosts`.

### Running the system

1. Poweroff all the BeagleBone.
2. Create ad ad-hoc WiFi network named 'gemb-hoc' on channel 1 from your Linux PC. Se the PC ip address to 192.168.200.51/24
3. Power on all the BeagleBone. Everything will start automatically.
4. From the `gui/` folder launch the GUI with `./build/greeneyes-gui`.
5. From the `vsn/` folder launch the SINK node with `./build-host/greeneyes-vsn sink 1 --telos /dev/ttyUSB0`
6. From the GUI control the system and enjoy your running Visual Senso Network

## Credits


