# BeaconScannerJNI
The native JNI code for the Java version of the beacon scanner

This relies on the bluez debian packages:
sudo apt-get install bluez bluez-hcidump libbluetooth-dev 

rather than building the bluez libraries directly as had been done with the NativeRaspberryPiBeaconParser project.

In addition, I have been using the oracle-java8-jdk package for the javavm.
