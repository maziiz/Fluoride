//
//  Copyright (C) 2017 Google, Inc.
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

#pragma once

#include <base/macros.h>
#include <hardware/bluetooth.h>
#include <hardware/bt_hearing_aid.h>

#include <vector>

namespace bluetooth {
namespace hal {

//class BluetoothHaInterface {
// public:
//  class HaObserver {
//   public:
//	  /** Callback for profile connection state change */
//	  virtual void OnConnectionState(ConnectionState state,
//	                                 const RawAddress& address);
//	  /** Callback for device being available. Is executed when devices are loaded
//	   * from storage on stack bringup, and when new device is connected to profile.
//	   * Main purpose of this callback is to keep its users informed of device
//	   * capabilities and hiSyncId.
//	   */
//	  virtual void OnDeviceAvailable(uint8_t capabilities, uint64_t hiSyncId,
//	                                 const RawAddress& address);
//
//   protected:
//    virtual ~HaObserver() = default;
//  };


  static bool Initialize();
  static void CleanUp();
  static bool IsInitialized();
//  static void InitializeForTesting(BluetoothAvInterface* test_instance);

//  static BluetoothHaInterface* Get();

//  virtual bool A2dpSourceEnable(
//      std::vector<btav_a2dp_codec_config_t> codec_priorities) = 0;
//  virtual void A2dpSourceDisable() = 0;
  virtual bool HaEnable();

//
//  virtual void AddA2dpSourceObserver(A2dpSourceObserver* observer) = 0;
//  virtual void RemoveA2dpSourceObserver(A2dpSourceObserver* observer) = 0;
//  virtual void AddA2dpSinkObserver(A2dpSinkObserver* observer) = 0;
//  virtual void RemoveA2dpSinkObserver(A2dpSinkObserver* observer) = 0;
//
  virtual const hearing_aid::HearingAidInterface* GetHaHALInterface() = 0;

 protected:
  BluetoothHaInterface() = default;
  virtual ~BluetoothHaInterface() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothHaInterface);
};

}  // namespace hal
}  // namespace bluetooth
