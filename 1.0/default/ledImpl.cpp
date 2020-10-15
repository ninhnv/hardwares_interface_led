#define LOG_TAG "LedService"

#include <log/log.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <cutils/uevent.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "ledImpl.h"

#define FDSIZE      1000
#define EPOLLEVENTS 100

const int kDefaultGpioPortOffset = 0; //458

// GPIO sysfs path
const char* const kGPIOSysfsPath = "/sys/class/gpio";
const int kPinLed = 17;  // Number(gpio17)  = kDefaultGpioPortOffset + kPinLed = 458 + 17 = 475
const int kPinSW = 18;   // Number(gpio18)  = kDefaultGpioPortOffset + kPinSW = 458 + 18 = 476

namespace android {
namespace hardware {
namespace led {
namespace V1_0 {
namespace implementation {

// Set by the signal handler to destroy the thread
volatile bool destroyThread;

ledImpl::ledImpl() {
	std::ostringstream gpio_export_path, gpio_led_port, gpio_sw_port;
	std::ostringstream gpio_led_direction; 
	std::ostringstream gpio_sw_direction;
	std::ostringstream gpio_sw_edge;
	int hGpioexport = -1;
	int hGpioLedDir = -1;
	int hGpioSWDir = -1;
	int hGpioSWEdge = -1;
	int ret = -1;
	
	valid = 1;
	state = LedStatus::LED_OFF;
	
	// Exit by any error
	do
	{
	
		//gpio_export_path = base::StringPrintf("%s/export", kGPIOSysfsPath);
		gpio_export_path << kGPIOSysfsPath << "/export";
		hGpioexport = open(gpio_export_path.str().c_str(),O_WRONLY);
		if(hGpioexport < 0)
		{
			ALOGE("LedGPIO Export Init Failed (%s)", gpio_export_path.str().c_str());
			valid = -1;
			break;
		}
		
		ALOGI("LedGPIO Export Init Successfully (%s)", gpio_export_path.str().c_str());
			
		// Add export for LED
		// Number(gpio17)  = kDefaultGpioPortOffset + kPinLed = 458 + 17 = 475
		// gpio_led_port = base::StringPrintf("%d", kDefaultGpioPortOffset + kPinLed);
		gpio_led_port << (kDefaultGpioPortOffset + kPinLed);
		ret = write(hGpioexport, gpio_led_port.str().c_str(), gpio_led_port.str().size());
		if(ret < 0)
		{
			ALOGE("LedGPIO Led Port Init Failed");
			valid = -1;
			break;
		}
		
		ALOGI("LedGPIO Led Port Init Successfully");
		// Define the direction(OUT) of LED port
		// gpio_led_direction = base::StringPrintf("%s/gpio%d/direction", kGPIOSysfsPath, kDefaultGpioPortOffset + kPinLed);
		gpio_led_direction << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinLed) <<"/direction";
		hGpioLedDir = open(gpio_led_direction.str().c_str(), O_WRONLY);
		if(hGpioLedDir < 0)
		{
			ALOGE("LedGPIO Led direction Init Failed (%s)", gpio_led_direction.str().c_str());
			valid = -1;
			break;
		}
		ALOGI("LedGPIO Led direction Init Successfully (%s)", gpio_led_direction.str().c_str());
		
		ret = write(hGpioLedDir, "out", strlen("out"));  
		if(ret < 0)
		{
			ALOGE("LedGPIO Led Port direction Init Failed");
			valid = -1;
			break;
		}
		ALOGI("LedGPIO Led Port direction Init Successfully");
		
		// Add export for SW
		// Number(gpio18)  = kDefaultGpioPortOffset + kPinSW = 458 + 18 = 476
		// gpio_sw_port = base::StringPrintf("%d", kDefaultGpioPortOffset + kPinSW);
		gpio_sw_port << (kDefaultGpioPortOffset + kPinSW);
		ret = write(hGpioexport, gpio_sw_port.str().c_str(), gpio_sw_port.str().size());
		if(ret < 0)
		{
			ALOGE("LedGPIO SW Port Init Failed");
			valid = -1;
			break;
		}
		ALOGI("LedGPIO SW Port Init Successfully");
		
		// Define the direction(IN) of SW port
		// gpio_sw_direction = base::StringPrintf("%s/gpio%d/direction", kGPIOSysfsPath, kDefaultGpioPortOffset + kPinSW);
		gpio_sw_direction << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinSW) << "/direction";
		hGpioSWDir = open(gpio_sw_direction.str().c_str(), O_WRONLY);
		if(hGpioSWDir < 0)
		{
			ALOGE("LedGPIO SW direction Init Failed");
			valid = -1;
			break;
		}
		ALOGI("LedGPIO SW direction Init Successfully");
		
		ret = write(hGpioSWDir, "in" , strlen("in"));
		if(ret < 0)
		{
			ALOGE("LedGPIO SW Port direction Init Failed");
			valid = -1;
			break;
		}
		ALOGI("LedGPIO SW Port direction Init Successfully");

		// Define the edge(rising) of SW port
		gpio_sw_edge << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinSW) << "/edge";
		hGpioSWEdge = open(gpio_sw_edge.str().c_str(), O_WRONLY);
		if(hGpioSWEdge < 0)
		{
			ALOGE("%s Init Failed", gpio_sw_edge.str().c_str());
			valid = -1;
			break;
		}
		ALOGI("%s Init Successfully", gpio_sw_edge.str().c_str());
		
		ret = write(hGpioSWEdge, "rising" , strlen("rising"));
		if(ret < 0)
		{
			ALOGE("LedGPIO SW Edge rising Init Failed");
			valid = -1;
			break;
		}
		ALOGI("LedGPIO SW Edge rising Init Successfully");
		
	}while(0);
		
	// Close all the local file handle
	// hGpioexport, hGpioLedDir, hGpioSWDir;

	if(hGpioexport >=0)
	{
		close(hGpioexport);
	}
	
	if(hGpioLedDir >=0)
	{
		close(hGpioLedDir);
	}
	
	if(hGpioSWDir >=0)
	{
		close(hGpioSWDir);
	}
	
	if(hGpioSWEdge >=0)
	{
		close(hGpioSWEdge);
	}

	if(valid >=0)
	{
		ALOGI("LedGPIO Init Successfully, valid:%d", valid);
	}
	else
	{
		ALOGE("LedGPIO Init Failed, valid:%d", valid);
	}

}

ledImpl::~ledImpl() {
	// Release gpio
	// $ echo 475 > /sys/class/gpio/unexport
	// $ echo 476 > /sys/class/gpio/unexport
	std::ostringstream gpio_unexport_path, gpio_led_port, gpio_sw_port;
	int hGpiounexport = -1;
	int hGpioLedDir = -1;
	int hGpioSWDir = -1;
	int ret = -1;

	do{
	if(1 == valid)
	{
		gpio_unexport_path << kGPIOSysfsPath << "/unexport";
		hGpiounexport = open(gpio_unexport_path.str().c_str(),O_WRONLY);
		if(hGpiounexport < 0)
		{
			ALOGE("led GPIO Unexport Init Failed (%s)", gpio_unexport_path.str().c_str());
			valid = -1;
			break;
		}
		
		ALOGI("led GPIO Unexport Init Successfully (%s)", gpio_unexport_path.str().c_str());

		// Unexport for LED
		// Number(gpio17)  = kDefaultGpioPortOffset + kPinLed = 458 + 17 = 475
		gpio_led_port << (kDefaultGpioPortOffset + kPinLed);
		ret = write(hGpiounexport, gpio_led_port.str().c_str(), gpio_led_port.str().size());
		if(ret < 0)
		{
			ALOGE("led GPIO Led Port Unexport Failed");
			valid = -1;
			break;
		}
		
		ALOGI("led GPIO Led Port Unexport Successfully");

		// Unexport for SW
		// Number(gpio18)  = kDefaultGpioPortOffset + kPinSW = 458 + 18 = 476
		gpio_sw_port << (kDefaultGpioPortOffset + kPinSW);
		ret = write(hGpiounexport, gpio_sw_port.str().c_str(), gpio_sw_port.str().size());
		if(ret < 0)
		{
			ALOGE("led GPIO SW Port Unexport Failed");
			valid = -1;
			break;
		}
		ALOGI("led GPIO SW Port Unexport Successfully");
	}
	}while(0);

	// Close all the local file handle
	// hGpioexport, hGpioLedDir, hGpioSWDir;

	if(hGpiounexport >=0)
	{
		close(hGpiounexport);
	}
	
	if(hGpioLedDir >=0)
	{
		close(hGpioLedDir);
	}
	
	if(hGpioSWDir >=0)
	{
		close(hGpioSWDir);
	}
}

Return<void> ledImpl::on() {
	//state = LedStatus::LED_ON;
	//ALOGE("ledImpl on status:%d", state);
    //return Void();
	int hGpioLed = -1;  // the handle of LED port
	int ret = -1;
	std::ostringstream gpio_led_value; 

	if(valid < 0)
	{
		ALOGE("led GPIO is unavailable!");
		return Void();
	}

	// gpio_led_value = base::StringPrintf("%s/gpio%d/value", kGPIOSysfsPath, kDefaultGpioPortOffset + kPinLed);
	gpio_led_value << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinLed) << "/value";
	hGpioLed = open(gpio_led_value.str().c_str(), O_WRONLY);
	if(hGpioLed < 0)
	{
		ALOGE("led GPIO %s is unavailable!", gpio_led_value.str().c_str());
		return Void();
	}
	

	ret = write(hGpioLed, "1", strlen("1"));
	if(ret < 0)
	{
		ALOGE("led GPIO Port write on Failed");
		//state = LedStatus::LED_BAD_VALUE;
	}

	close(hGpioLed);
	
	//state = LedStatus::LED_ON;
	ALOGI("led GPIO On, status:%d", state);
	return Void();
}

Return<void> ledImpl::off() {
	//state = LedStatus::LED_OFF;
	//ALOGE("ledImpl off status:%d", state);	
    //return Void();
	int hGpioLed = -1;  // the handle of LED port
	int ret = -1;
	std::ostringstream gpio_led_value; 
	
	if(valid < 0)
	{
		ALOGE("led GPIO is unavailable! valid=%d",valid);
		return Void();
	}

	// gpio_led_value = base::StringPrintf("%s/gpio%d/value", kGPIOSysfsPath, kDefaultGpioPortOffset + kPinLed);
	gpio_led_value << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinLed) << "/value";
	hGpioLed = open(gpio_led_value.str().c_str(), O_WRONLY);
	if(hGpioLed < 0)
	{
		ALOGE("led GPIO %s is unavailable!", gpio_led_value.str().c_str());
		return Void();
	}
	

	ret = write(hGpioLed, "0", strlen("0"));
	if(ret < 0)
	{
		ALOGE("led GPIO Port write off Failed");
		//state = LedStatus::LED_BAD_VALUE;
	}

	close(hGpioLed);
    
	//state = LedStatus::LED_OFF;    
	ALOGI("led GPIO Off, status:%d", state);
	return Void();
}

Return<LedStatus> ledImpl::get() {
	// TODO implement
	pthread_mutex_lock(&mutexSW); //mutex lock

	pthread_cond_wait(&conditionSW, &mutexSW); //wait for the SW(gpio 18) pressed

	if (LedStatus::LED_ON == state){
		state = LedStatus::LED_OFF;
	}
	else{
		state = LedStatus::LED_ON;
	}

	pthread_mutex_unlock(&mutexSW);
	return state;
	//return ::android::hardware::led::V1_0::LedStatus {};
}
Return<int32_t> ledImpl::set(LedStatus val) {
	if(val == LedStatus::LED_OFF || val == LedStatus::LED_ON)
		state = val;
	else
		return -1;
	return 0;
}

Return<void> ledImpl::getBrightnessRange(getBrightnessRange_cb _hidl_cb)
{
	ALOGE("ledImpl getBrightnessRange ");
	BrightnessRange range;
	range.max = 100;
	range.min = 1;
	_hidl_cb(true,range);
	return Void();
	
}
  
Return<bool> ledImpl::setBrightnessValue(const hidl_vec<int32_t>& range)
{
	ALOGE("ledImpl getBrightnessValue ");
	auto iter = range.begin();
	ALOGE("ledImpl getBrightnessValue range.begin: %d",*iter);
	iter = range.end();
	ALOGE("ledImpl getBrightnessValue range.end: %d",*iter);
	ALOGE("ledImpl getBrightnessValue range.size: %zu",range.size());
	return true;
}

void* work(void* param) {
    int epoll_fd;
    int hGpioSWvalue = -1;   // the handle of SW Value
    struct epoll_event ev;
    int swEvents = 0;
    ledImpl* pParent;
    char gpioValue[2];
    int ret = 0;
    //int cnt = 0;

    ALOGD("creating thread...");

    do{   // exit by error
	std::ostringstream gpio_sw_value;

	if (NULL == param){
		ALOGE("Create thread failed: param == NULL");
		break;
	}
	pParent = (ledImpl*) param;
	ALOGD("Created thread!");

	gpio_sw_value << kGPIOSysfsPath << "/gpio" << (kDefaultGpioPortOffset + kPinSW) << "/value";
	hGpioSWvalue = open(gpio_sw_value.str().c_str(), O_RDONLY);
	if(hGpioSWvalue < 0)
	{
		ALOGE("led GPIO SW Value Init Failed");
		break;
	}
	ALOGD("%s is ready.", gpio_sw_value.str().c_str());
    
	// EPOLLPRI and gpio edge should be used, because read gpio cannot block and return immdiately.
	ev.events = EPOLLPRI | EPOLLERR;    // Poll the sw gpio value
	ev.data.fd = hGpioSWvalue;

	epoll_fd = epoll_create(FDSIZE);
	if (epoll_fd == -1) {
		ALOGE("epoll_create failed; errno=%d", errno);
		break;
	}
	ALOGD("Created epoll!");

	// add listener

	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, hGpioSWvalue, &ev) == -1) {
		ALOGE("epoll_ctl failed; errno=%d", errno);
		break;
	}
	ALOGD("Created epoll listener!");

	while (!destroyThread) {
		struct epoll_event events[EPOLLEVENTS];

		memset(gpioValue, 0, sizeof(gpioValue));
		ALOGD("Before usb epoll_wait; swEvnet=%d", swEvents);
		swEvents = epoll_wait(epoll_fd, events, EPOLLEVENTS, -1);  // -1 Timeout infinitely
		ALOGD("After usb epoll_wait; swEvnet=%d", swEvents);

		if (swEvents == -1) {
		    if (errno == EINTR)
			continue;
		    ALOGE("usb epoll_wait failed; errno=%d", errno);
		    break;
		}
		for (int n = 0; n < swEvents; ++n) {
		    if (events[n].data.fd){  // == hGpioSWvalue?
		        // Get gpio sw value
			lseek(hGpioSWvalue, 0, SEEK_SET);  
			ret = read(hGpioSWvalue, gpioValue, 1);
			ALOGD("SW Value = %s",gpioValue);
			// Callback
			// pParent->mCallback->sendEvent(cnt++);

			//Unblock Return<LedStatus> ledImpl::get()
			pthread_cond_signal(&(pParent->conditionSW)); 
		    }
		}
	}


    }while(0);


    if (hGpioSWvalue > 0) {
        close(hGpioSWvalue);
    }
    return NULL;
}

void sighandler(int sig)
{
    if (sig == SIGUSR1) {
        destroyThread = true;
        ALOGI("destroy set");
        return;
    }
    signal(SIGUSR1, sighandler);
}

ILed* HIDL_FETCH_ILed(const char * /*name*/) {
	ALOGE("ledImpl HIDL_FETCH_ILed ");	
    return new ledImpl();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace led
}  // namespace hardware
}  // namespace android