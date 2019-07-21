//
//  Copyright 2015 Google, Inc.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at:
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

#include "service/daemon.h"

#include <memory>

#include <base/logging.h>
#include <base/run_loop.h>
#include <hardware/bt_hearing_aid.h>
#include <hardware/hardware.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include "stack/include/btu.h"
#include "osi/include/alarm.h"

#include "service/adapter.h"
#include "service/hal/bluetooth_av_interface.h"
#include "service/hal/bluetooth_avrcp_interface.h"
#include "service/hal/bluetooth_gatt_interface.h"
#include "service/hal/bluetooth_interface.h"
#include "service/ipc/ipc_manager.h"
#include "service/settings.h"
#include "service/switches.h"
#include "BlockingRecord.h"

//#include "ServerSide.cc"
using bluetooth::hearing_aid::ConnectionState;
using bluetooth::hearing_aid::HearingAidInterface;
int flag_bond_conn = 1;
extern struct audio_module HAL_MODULE_INFO_SYM;
pthread_t Record_thread;

int SamplingRate = 16000;
int FrameDuration = 40;	//ms // frame duration must be 40 msec to record 640 bytes
int bit_Depth = 16;
int Channels = 1;

audio_hw_device_t* mDev;
audio_stream_out_t* streamOut;

namespace bluetooth {

namespace {
const std::string DesiredDeviceName = "nimble_R_asha"; //Synopsys_BLE_Dev  OPPO A57 nimble-bleprph nimble_server
const std::string DesiredDeviceAddress = "00:00:99:99:00:00";

// The global Daemon instance.
Daemon* g_daemon = nullptr;

class DaemonImpl: public Daemon,
		public ipc::IPCManager::Delegate,
		public bluetooth::Adapter::Observer,
		public bluetooth::hearing_aid::HearingAidCallbacks {
public:
	DaemonImpl() :
			initialized_(false) {
	}

	~DaemonImpl() override {
		if (!initialized_)
			return;

		CleanUpBluetoothStack();
	}

	void StartMainLoop() override {
		base::RunLoop().Run();
	}
	bool SetScanEnable(bool scan_enable) override {
		return adapter_->SetScanEnable(scan_enable);
	}
	bool CreateBond(const std::string& device_address, int transport) override {
		return adapter_->CreateBond(device_address, transport);
	}
	bool RemoveBond(const std::string& device_address) override {
		return adapter_->RemoveBond(device_address);
	}
	virtual void OnConnectionState(ConnectionState state,
			const RawAddress& address) override {
		LOG(INFO) << "DEVICE IS  " << (int) state;
		int rc;
		const char* address_ = "1";
		if ((int) state == 2) {
//		  LOG(INFO) << __func__ << "thread id"<< (int)pthread_self();
			if (pthread_create(&Record_thread, NULL, Record_thread_func, NULL)
					!= 0) {
				perror("pthread_create");
			}
//    	rc = OpenOutputStream(address_, true /*mono*/, 16000, &streamOut);
//    	LOG(INFO) << "status = "<< rc;
//	    WriteSomethingIntoStream(streamOut,640, 100000);
////	    mDev->close_output_stream(mDev, streamOut);
		}
	}

	virtual void OnDeviceAvailable(uint8_t capabilities, uint64_t hiSyncId,
			const RawAddress& address) override {
		LOG(INFO) << "DEVICE IS available with address " << address;
	}

	Settings* GetSettings() const override {
		return settings_.get();
	}

	base::MessageLoop* GetMessageLoop() const override {
		return message_loop_.get();
	}

private:
	hearing_aid::HearingAidInterface* HearingAid_iface = nullptr;

	// ipc::IPCManager::Delegate implementation:
	void OnIPCHandlerStarted(ipc::IPCManager::Type /* type */) override {
		if (!settings_->EnableOnStart())
			return;
		adapter_->Enable(false /* start_restricted */);
		LOG(INFO) << "enabled";
	}

	void OnIPCHandlerStopped(ipc::IPCManager::Type /* type */) override {
		// Do nothing.
	}

	bool StartUpBluetoothInterfaces() {
		if (!hal::BluetoothInterface::Initialize())
			goto failed;

		if (!hal::BluetoothGattInterface::Initialize())
			goto failed;

//    if (!hal::BluetoothHaInterface::Initialize()) goto failed;

//    if (!hal::BluetoothAvrcpInterface::Initialize()) goto failed;

		return true;

		failed: ShutDownBluetoothInterfaces();
		return false;
	}

	void ShutDownBluetoothInterfaces() {
		if (hal::BluetoothGattInterface::IsInitialized())
			hal::BluetoothGattInterface::CleanUp();
		if (hal::BluetoothInterface::IsInitialized())
			hal::BluetoothInterface::CleanUp();
		if (hal::BluetoothAvInterface::IsInitialized())
			hal::BluetoothAvInterface::CleanUp();
		if (hal::BluetoothAvrcpInterface::IsInitialized())
			hal::BluetoothAvrcpInterface::CleanUp();
	}

	void CleanUpBluetoothStack() {
		// The Adapter object needs to be cleaned up before the HAL interfaces.
		ipc_manager_.reset();
		adapter_.reset();
		ShutDownBluetoothInterfaces();
	}

	bool SetUpIPC() {
		// If an IPC socket path was given, initialize IPC with it. Otherwise
		// initialize Binder IPC.
		if (settings_->UseSocketIPC()) {
			if (!ipc_manager_->Start(ipc::IPCManager::TYPE_LINUX, this)) {
				LOG(ERROR)
										<< "Failed to set up UNIX domain-socket IPCManager";
				return false;
			}
			return true;
		}

#if !defined(OS_GENERIC)
		if (!ipc_manager_->Start(ipc::IPCManager::TYPE_BINDER, this)) {
			LOG(ERROR) << "Failed to set up Binder IPCManager";
			return false;
		}
#else
		if (!ipc_manager_->Start(ipc::IPCManager::TYPE_DBUS, this)) {
			LOG(ERROR) << "Failed to set up DBus IPCManager";
			return false;
		}
#endif

		return true;
	}
	static void *Record_thread_func(void *param) {
		int SampleBytes = ((int) bit_Depth / (int) 8); // 2
		int Frame_Size = ((int) SampleBytes * (int) Channels); // 2
		int NumFrames =
				(((int) SamplingRate * (int) FrameDuration) / (int) 1000); // 16000*10/1000= 160/320
		int BufferSize = ((int) NumFrames * (int) Frame_Size); // 160,000 * 2

		char* buf;
		PaStream* st = NULL;
		int error = -1;
		int i;

		const char* address_ = "1";
		int rc;

		st = Start_Recording(st, NumFrames, SamplingRate, Channels, bit_Depth);
		if (st == NULL)
			LOG(FATAL) << "Cannot start recording";

		buf = (char*) malloc(BufferSize);    // recording buffer
		memset(buf, 0, BufferSize);					// remark
		rc = OpenOutputStream(address_, true /*mono*/, 16000, &streamOut);
		FILE  *fid;
	    fid = fopen("recorded.raw", "wb");
	    int first_time= 0;
		for (;;) {
			rc = Pa_ReadStream(st, buf, NumFrames);
			if (rc) {
				LOG(FATAL) << "CANNOT RECORD" << "RC = " << rc;
			}
//			for(i=0;i<320;i++){
//			   printf("element #%d is %d\n",i,(uint8_t)buf[i]);
//			}

//			if( fid == NULL )
//			   {
//			       printf("Could not open file.");
//			   }
//			   else
//			   {
//				   size_t written_Data = fwrite((const char*) buf, Frame_Size, NumFrames, fid );
//			       printf("Wrote data to 'recorded.raw %d'\n",written_Data);
//			   }

			LOG(INFO) << "BUFFER SIZE = : " << BufferSize;
			streamOut->write(streamOut, (const char*) buf, BufferSize);

		}
		Stop_Recording(st);
		return NULL;
	}
	static int load_audio_interface(const char* if_name,
			audio_hw_device_t **dev) {
		const hw_module_t *mod;
		int rc=0;

//      rc = hw_get_module_by_class(AUDIO_HARDWARE_MODULE_ID, if_name, &mod);
		mod = &HAL_MODULE_INFO_SYM.common;
		if (rc) {
			LOG(ERROR) << " couldn't load audio hw module" << __func__
									<< " "
									AUDIO_HARDWARE_MODULE_ID << " " << if_name;
			goto out;
		}
		rc = audio_hw_device_open(mod, dev);
		if (rc) {
			LOG(ERROR) << " couldn't open audio hw device in " << __func__
									<< " "
									AUDIO_HARDWARE_MODULE_ID << " " << if_name;
			goto out;
		}
		if ((*dev)->common.version < AUDIO_DEVICE_API_VERSION_MIN) {
			LOG(ERROR) << "wrong audio hw device version" << __func__ << " "
									<< (*dev)->common.version;
			rc = -1;
			audio_hw_device_close(*dev);
			goto out;
		}
		return 0;

		out: *dev = NULL;
		return rc;
	}

	void send_data(void*) {
//	 int rc ;
//	 const char* address_ = "1";
//	 LOG(INFO) << __func__ << "thread id"<< (int)pthread_self();
//    	rc = OpenOutputStream(address_, true /*mono*/, 48000, &streamOut);
//     LOG(INFO) << "status = "<< rc;
//	   WriteSomethingIntoStream(streamOut,2, 1);
//	    mDev->close_output_stream(mDev, streamOut);
	}
	static int OpenOutputStream(const char* address, bool mono,
			uint32_t sampleRate, audio_stream_out_t** streamOut) {
		*streamOut = nullptr;
		int rc;
		struct audio_config configOut = { };
		configOut.channel_mask =
				mono ? AUDIO_CHANNEL_OUT_MONO : AUDIO_CHANNEL_OUT_STEREO;
		configOut.sample_rate = sampleRate;
		rc = mDev->open_output_stream(mDev, AUDIO_IO_HANDLE_NONE,
				AUDIO_DEVICE_NONE, AUDIO_OUTPUT_FLAG_NONE, &configOut,
				streamOut, address);
		return rc;
	}
	void WriteIntoStream(audio_stream_out_t* streamOut, const char* buffer,
			size_t bufferSize) {
		int rc;
		ssize_t result = streamOut->write(streamOut, buffer, bufferSize);
	}
	void GenerateData(char* buffer, size_t bufferSize) {
		for (size_t i = 0; i < bufferSize; ++i) {
			buffer[i] = static_cast<char>(i & 0x7f);
		}
	}
	void WriteSomethingIntoStream(audio_stream_out_t* streamOut,
			size_t bufferSize, size_t repeats) {
		std::unique_ptr<char[]> buffer(new char[bufferSize]);
		GenerateData(buffer.get(), bufferSize);
		for (size_t i = 0; i < repeats; ++i) {
			WriteIntoStream(streamOut, buffer.get(), bufferSize);
		}
	}
	bool Init() override {
		CHECK(!initialized_);
		message_loop_.reset(new base::MessageLoop());
		settings_.reset(new Settings());
		if (!settings_->Init()) {
			LOG(ERROR) << "Failed to set up Settings";
			return false;
		}
		LOG(INFO) << "after setting init";
		if (!StartUpBluetoothInterfaces()) {
			LOG(ERROR) << "Failed to set up HAL Bluetooth interfaces";
			return false;
		}
		LOG(INFO) << "after start up bt";
		adapter_ = Adapter::Create();
		adapter_->AddObserver(this);
		ipc_manager_.reset(new ipc::IPCManager(adapter_.get()));

		if (!SetUpIPC()) {
			CleanUpBluetoothStack();
			return false;
		}

		initialized_ = true;
		LOG(INFO) << "Daemon initialized";
		load_audio_interface("Hearing Aid Audio HW HAL", &mDev);
		HearingAid_iface =
				(hearing_aid::HearingAidInterface*) (hal::BluetoothInterface::Get()->GetHALInterface()->get_profile_interface(
				BT_PROFILE_HEARING_AID_ID));
		LOG(INFO) << "AFTER GET HA ID";

		if (!HearingAid_iface) {
			return false;
		}
		return true;
	}

	void OnAdapterStateChanged(Adapter* adapter, AdapterState prev_state,
			AdapterState new_state) override {
		LOG(INFO) << "state changed Daemon impl";
		if (new_state == ADAPTER_STATE_ON) {
			if (!SetScanEnable(true)) {
				LOG(ERROR) << "CANNOT START SCANNING"; //this found on the log
			};
			HearingAid_iface->Init(this);
		}
		// Default implementation does nothing
	}
	void OnDeviceConnectionStateChanged(Adapter* adapter,
			const std::string& device_address, bool connected) override {
		if (connected && flag_bond_conn) {
//		  RemoveBond(DesiredDeviceAddress);
			flag_bond_conn = 0;
		}

	}
	void OnDeviceFound(Adapter* adapter, const RemoteDeviceProps& props)
			override {
		LOG(INFO) << "device found Daemon impl";
		LOG(INFO) << "Device address: " << props.address();
		LOG(INFO) << "Device name: " << props.name();
		if (props.name() == DesiredDeviceName) {
			LOG(INFO) << "the desired device found"; //this found on the log
			if (!SetScanEnable(false)) {
				LOG(ERROR) << "CANNOT STOP SCANNING"; //this found on the log
			};
//	    if(!CreateBond(props.address(),2)){
//		    LOG(ERROR) << "CANNOT CONNECT WITH THE SPECIFIC DEVICE ADDRESS"; //this found on the log
//	    }
			RawAddress bd_addr;
			RawAddress::FromString(DesiredDeviceAddress, bd_addr);
			HearingAid_iface->Connect(bd_addr);

		}
	}
	bool initialized_;
	std::unique_ptr<base::MessageLoop> message_loop_;
	std::unique_ptr<Settings> settings_;
	std::unique_ptr<Adapter> adapter_;
	std::unique_ptr<ipc::IPCManager> ipc_manager_;

	DISALLOW_COPY_AND_ASSIGN(DaemonImpl)
	;

};

}  // namespace
// static
bool Daemon::Initialize() {
	CHECK(!g_daemon);

	g_daemon = new DaemonImpl();
	if (g_daemon->Init())
		return true;
	LOG(ERROR) << "Failed to initialize the Daemon object";

	delete g_daemon;
	g_daemon = nullptr;

	return false;
}

void Daemon::ShutDown() {
	CHECK(g_daemon);
	delete g_daemon;
	g_daemon = nullptr;
}

// static
void Daemon::InitializeForTesting(Daemon* test_daemon) {
	CHECK(test_daemon);
	CHECK(!g_daemon);

	g_daemon = test_daemon;
}

// static
Daemon* Daemon::Get() {
	CHECK(g_daemon);
	return g_daemon;
}

}  // namespace bluetooth
