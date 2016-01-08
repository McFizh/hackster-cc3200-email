# hackster-cc3200-email
This repo contains code for PHP backend, Energia patches, and code for CC3200 board needed to run this project. Backend requires PHP version of 5.5 or later with SQLite, Imap and SSL support.

### Patching Energia
Copy the two provided files in "energiaPatches" folder to "hardware/cc3200/libraries/WiFi" folder which is under energias installation folder. These small changes allow cc3200 to use user certificates, which are needed to authenticate to AWS IOT

### Installing Energia libraries
Copy content of "energiaLibs" folder to library folder which lives inside Energias workspace folder. On linux the folder is something like "/home/username/sketchbook/libraries".  

Note: do not copy these to the application data folder, in which you copied the patches to.

