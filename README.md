# GRiDISK Commander
GRiDISK Commander - File manager for GRiD OS standard disks, used in GRiD Compass laptops.

![Main Window](https://raw.githubusercontent.com/Bs0Dd/GRiDISKCOM/main/mainwindow.png)

Commander can:

* Create new images.
* Add files to the image.
* Create/add folders to the image.
* Copy files/folders inside the image.
* Transfer files between images.
* Dump files to the computer.
* Rename files and folders.
* Change image label.
* Work with GRiD HDD (with MBR) partitions and extract them.
* Work with Bubble Memory images.

## Building requirements
* Needs MinGW x32 or x64 with Qt5 to compile in Windows.
* Needs CMake with Qt5 to compile in Linux.

Don't forget to initialize the submodule:
```
git submodule init
git submodule update
```

## Special thanks to
* [BOOtak](https://github.com/BOOtak) for first testing, bugfixes and [core library](https://github.com/BOOtak/CCOS-disk-utils) for this application.
* [vklachkov](https://github.com/vklachkov) for the current project maintenance.
* [usernameak](https://github.com/usernameak) and other people for developing the GRiD Compass emulator (for MAME), which helped test the disks.
