/******************************************************************************
 * Copyright 2019 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

#pragma once

#include <memory>
#include <thread>

#include "modules/canbus/proto/canbus_conf.pb.h"
#include "modules/canbus/proto/vehicle_parameter.pb.h"
#include "modules/common_msgs/basic_msgs/error_code.pb.h"
#include "modules/common_msgs/chassis_msgs/chassis.pb.h"
#include "modules/common_msgs/control_msgs/control_cmd.pb.h"

#include "modules/canbus/vehicle/vehicle_controller.h"
#include "modules/canbus_vehicle/ch/protocol/brake_command_111.h"
#include "modules/canbus_vehicle/ch/protocol/gear_command_114.h"
#include "modules/canbus_vehicle/ch/protocol/steer_command_112.h"
#include "modules/canbus_vehicle/ch/protocol/throttle_command_110.h"
#include "modules/canbus_vehicle/ch/protocol/turnsignal_command_113.h"
#include "modules/canbus_vehicle/ch/protocol/vehicle_mode_command_116.h"

namespace apollo {
namespace canbus {
namespace ch {

class ChController final : public VehicleController<::apollo::canbus::Ch> {
 public:
  ChController() {}

  virtual ~ChController();

  ::apollo::common::ErrorCode Init(
      const VehicleParameter& params,
      CanSender<::apollo::canbus::Ch>* const can_sender,
      CanReceiver<::apollo::canbus::Ch>* const can_receiver,
      MessageManager<::apollo::canbus::Ch>* const message_manager) override;

  bool Start() override;

  /**
   * @brief stop the vehicle controller.
   */
  void Stop() override;

  /**
   * @brief calculate and return the chassis.
   * @returns a copy of chassis. Use copy here to avoid multi-thread issues.
   */
  Chassis chassis() override;

  /**
   * @brief ccheck the chassis detail received or lost.
   */
  bool CheckChassisCommunicationError();

  FRIEND_TEST(ChControllerTest, SetDrivingMode);
  FRIEND_TEST(ChControllerTest, Status);
  FRIEND_TEST(ChControllerTest, UpdateDrivingMode);

 private:
  // main logical function for operation the car enter or exit the auto driving
  void Emergency() override;
  ::apollo::common::ErrorCode EnableAutoMode() override;
  ::apollo::common::ErrorCode DisableAutoMode() override;
  ::apollo::common::ErrorCode EnableSteeringOnlyMode() override;
  ::apollo::common::ErrorCode EnableSpeedOnlyMode() override;

  // NEUTRAL, REVERSE, DRIVE
  void Gear(Chassis::GearPosition state) override;

  // brake with new acceleration
  // acceleration:0.00~99.99, unit:
  // acceleration_spd: 60 ~ 100, suggest: 90
  void Brake(double acceleration) override;

  // drive with old acceleration
  // gas:0.00~99.99 unit:
  void Throttle(double throttle) override;

  // drive with acceleration/deceleration
  // acc:-7.0~5.0 unit:m/s^2
  void Acceleration(double acc) override;

  // steering with old angle speed
  // angle:-99.99~0.00~99.99, unit:, left:+, right:-
  void Steer(double angle) override;

  // steering with new angle speed
  // angle:-99.99~0.00~99.99, unit:, left:+, right:-
  // angle_spd:0.00~99.99, unit:deg/s
  void Steer(double angle, double angle_spd) override;

  // set Electrical Park Brake
  void SetEpbBreak(const control::ControlCommand& command) override;
  common::ErrorCode HandleCustomOperation(
      const external_command::ChassisCommand& command) override;
  void SetBeam(const common::VehicleSignal& signal) override;
  void SetHorn(const common::VehicleSignal& signal) override;
  void SetTurningSignal(const common::VehicleSignal& signal) override;

  // response vid
  bool VerifyID() override;
  void ResetProtocol();
  bool CheckChassisError();
  bool CheckVin();
  void GetVin();
  void ResetVin();

 private:
  void SecurityDogThreadFunc();
  virtual bool CheckResponse(const int32_t flags, bool need_wait);
  void set_chassis_error_mask(const int32_t mask);
  int32_t chassis_error_mask();
  Chassis::ErrorCode chassis_error_code();
  void set_chassis_error_code(const Chassis::ErrorCode& error_code);

 private:
  // control protocol
  Brakecommand111* brake_command_111_ = nullptr;
  Gearcommand114* gear_command_114_ = nullptr;
  Steercommand112* steer_command_112_ = nullptr;
  Throttlecommand110* throttle_command_110_ = nullptr;
  Turnsignalcommand113* turnsignal_command_113_ = nullptr;
  Vehiclemodecommand116* vehicle_mode_command_116_ = nullptr;

  Chassis chassis_;
  std::unique_ptr<std::thread> thread_;
  bool is_chassis_error_ = false;

  std::mutex chassis_error_code_mutex_;
  Chassis::ErrorCode chassis_error_code_ = Chassis::NO_ERROR;

  std::mutex chassis_mask_mutex_;
  int32_t chassis_error_mask_ = 0;
  uint32_t lost_chassis_reveive_detail_count_ = 0;
  bool is_need_count_ = true;
  bool is_chassis_communication_error_ = false;
};

}  // namespace ch
}  // namespace canbus
}  // namespace apollo
