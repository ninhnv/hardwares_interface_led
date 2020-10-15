//#define LOG_NDEBUG 0
#define LOG_TAG "Led_GPIO_DEMOSERVICE"  


#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>  
#include "demoservice.h"

  
#include <utils/Log.h>  
#include "ledImpl.h"

using android::hardware::led::V1_0::ILed;
using android::hardware::led::V1_0::LedStatus;
using android::hardware::hidl_vec;
using android::sp;
using android::hardware::hidl_string;


namespace com {

namespace example {

namespace gpio_led {

DemoService::DemoService()
{
		mHaveNotify = false;
		mSwStatus = -1;
        sp<::android::hardware::led::V1_0::ILed> service = ILed::getService();
		if( service == nullptr ){
			ALOGE("Can't find ILed@1.0 service...");
		}
}

//virtual ::android::binder::Status DemoService::AddNumber(int a,int b)
::android::binder::Status DemoService::AddNumber(int32_t a, int32_t b, int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
    ALOGD("DemoService AddNumber a:%d, b:%d\n", a, b);
	//return a + b
    (*_aidl_return) = a + b;
    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
}

//virtual ::android::binder::Status DemoService::MaxNumber(int a,int b)
::android::binder::Status DemoService::MaxNumber(int32_t a, int32_t b, int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
	ALOGD("DemoService MaxNumber a:%d, b:%d\n", a, b);
    //return a > b ? a : b;
    (*_aidl_return) = (a > b ? a : b);
    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
}

//virtual ::android::binder::Status DemoService::SetLed(int32_t a)
::android::binder::Status DemoService::SetLed(int32_t a, int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
	sp<ILed> service = ILed::getService();
	String16 sOff("Calllback from servcie LedOff");
	String16 sOn("Calllback from servcie LedOn");

	if( service == nullptr ){
		ALOGE("Can't find ILed@2.0 service...");
		//return -1;
		_aidl_ret_status = ::android::UNKNOWN_ERROR;
		_aidl_status.setFromStatusT(_aidl_ret_status);
		return _aidl_status;
	}


	if(0 == a){
		//Turn off led
		ALOGI("ILed OFF");
		service->off();
	}
	else{
		//Turn on led
		ALOGI("ILed ON");
		service->on();
	}
	
	(*_aidl_return) = a;
	
    ALOGD("DemoService SetLed ");
    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
    //return 0;
}


::android::binder::Status DemoService::pollSW(int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
	sp<ILed> service = ILed::getService();	
	if( service == nullptr ){
		ALOGE("Can't find ILed@1.0 service...");
		//return -1;
		_aidl_ret_status = ::android::UNKNOWN_ERROR;
		_aidl_status.setFromStatusT(_aidl_ret_status);
		return _aidl_status;
	}

	ALOGD("Before pollSW->get()");
	pthread_mutex_lock(&mutexHidlSW); //mutex lock

	pthread_cond_wait(&conHidlSW, &mutexHidlSW); //wait for the SW(gpio 18) pressed
	(*_aidl_return) = mSwStatus;

	pthread_mutex_unlock(&mutexHidlSW);
	ALOGD("After pollSW->get()");


    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
}

::android::binder::Status DemoService::LedOn(int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
	sp<ILed> service = ILed::getService();

	if( service == nullptr ){
		ALOGE("Can't find ILed@1.0 service...");
		//return -1;
		_aidl_ret_status = ::android::UNKNOWN_ERROR;
		_aidl_status.setFromStatusT(_aidl_ret_status);
		return _aidl_status;
	}


	//Turn on led
	ALOGE("ILed ON");
	service->on();

	(*_aidl_return) = 1;

    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
}

::android::binder::Status DemoService::LedOff(int32_t* _aidl_return)
{
	::android::status_t _aidl_ret_status = ::android::OK;
	::android::binder::Status _aidl_status;
	sp<ILed> service = ILed::getService();

	if( service == nullptr ){
		ALOGE("Can't find ILed@1.0 service...");
		//return -1;
		_aidl_ret_status = ::android::UNKNOWN_ERROR;
		_aidl_status.setFromStatusT(_aidl_ret_status);
		return _aidl_status;
	}


	//Turn on led
	ALOGE("ILed OFF");
	service->off();
    
    (*_aidl_return) = 0;

    // ALOGD("DemoService SetLed ");
    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
    //return 0;
}

::android::binder::Status DemoService::regist(const ::android::sp<::com::example::gpio_led::ICallback>& cb) 
{
    ::android::status_t _aidl_ret_status = ::android::OK;
    ::android::binder::Status _aidl_status;
    // mNotifyDemoClient = cb;
	// mHaveNotify = true;

    ALOGD("DemoService regist cb.");
    _aidl_status.setFromStatusT(_aidl_ret_status);
    return _aidl_status;
}

}  // namespace gpio_led

}  // namespace example

}  // namespace com

using namespace com::example::gpio_led;

//int main(int argc, char** argv)  
int main()  
{  
	//ProcessState::initWithDriver("/dev/vndbinder");

    com::example::gpio_led::DemoService::instance();  
	//pthread_t hidlThread;
	//DemoService* sv = new DemoService();

	//defaultServiceManager()->addService(String16(SERVICE_NAME), sv);

    ProcessState::self()->startThreadPool();  
    IPCThreadState::self()->joinThreadPool();  
  
    return 0;  
} 
