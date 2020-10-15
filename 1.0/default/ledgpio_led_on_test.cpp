#define LOG_TAG "Led GPIO Test Client"
 
#include <android/hardware/led/1.0/ILed.h>
#include <hidl/Status.h>
#include <hidl/LegacySupport.h>
#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hidl/HidlSupport.h>
 
 
#include<stdio.h>
 
 
using android::hardware::led::V1_0::ILed;
using android::sp;
 
 
int main() {
      android::sp<ILed> service = ILed::getService();

    if( service == nullptr ){
        printf("Can't find ILed@1.0 service...\n");
    }

    service->on();
 
    printf("GPIO Led ON\n");
 
    return 0; 
}