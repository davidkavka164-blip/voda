#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
CXX=${CXX:-g++}
BUILD_DIR="/tmp/hwc_native_$$"
mkdir -p "$BUILD_DIR/portable" "$BUILD_DIR/nvs" "$BUILD_DIR/protection" "$BUILD_DIR/journal" "$BUILD_DIR/replay" "$BUILD_DIR/upper_volume_powercut" "$BUILD_DIR/history_exact_readback" "$BUILD_DIR/well_exact_readback"
trap 'rm -rf "$BUILD_DIR"' EXIT HUP INT TERM
CXXFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -std=c++11 -Wall -Wextra -Werror -pedantic -Itests/stubs -Ifirmware/common"

# Compile shared host modules once. This keeps the full suite practical while
# retaining the same strict compiler diagnostics for every test executable.
PORTABLE="PeerDatagramEnvelope PeerAuthenticatedStatusReceiver PeerCommandFrame PeerSessionCodec PeerStatusSessionCodec PeerStatusHmacCodec PeerRequestTracker PeerCommandSession PeerSequenceAllocator PersistentPeerReplayGuard PeerSessionKeys PeerCommandResponse PeerSecretVault HmacSha256 PeerCommandGuard PeerHandshake PeerHandshakeInitiator PeerHandshakeResponderCoordinator PeerHandshakeReady PeerHandshakeWire PeerHandshakeTimeout PeerHandshakeRetryPolicy TimedPeerHandshakeResponder TimedPeerHandshakeResponderCoordinator TimedPeerHandshakeInitiator TransferHistory ManualValveControl WaterMath TransferLogic TransferRestartGuard WellPumpProtection LowerTankPumpProtection SensorMath SensorConfig WellGeometry WellCalibrationEditor UpperTankVolumeCalibration WaterTemperature UpperTankLimits UpperTankProtection PeerProtocol InflowPersistencePolicy WellProtectionConfig PumpSafetyRelayOutput LowerTankInflowWatchdog"
for MODULE in $PORTABLE; do
  $CXX $CXXFLAGS -c "firmware/common/$MODULE.cpp" -o "$BUILD_DIR/portable/$MODULE.o"
done

NVS="NvsPeerSharedKeyStore NvsWellGeometryStore NvsWellCalibrationStore NvsDailyInflowStore NvsSensorConfigStore NvsUpperTankLimitsStore NvsUpperTankVolumeCalibrationStore NvsTransferHistoryStore NvsLowerTankInflowWatchdogStore"
for MODULE in $NVS; do
  if [ "$MODULE" = "NvsTransferHistoryStore" ] || [ "$MODULE" = "NvsWellCalibrationStore" ] || [ "$MODULE" = "NvsWellGeometryStore" ]; then continue; fi
  $CXX $CXXFLAGS -c "firmware/common/$MODULE.cpp" -o "$BUILD_DIR/nvs/$MODULE.o"
done
$CXX -Itests/stubs/history_exact_readback $CXXFLAGS -c firmware/common/NvsTransferHistoryStore.cpp -o "$BUILD_DIR/history_exact_readback/NvsTransferHistoryStore.o"
for MODULE in NvsWellCalibrationStore NvsWellGeometryStore; do
  $CXX -Itests/stubs/well_exact_readback $CXXFLAGS -c "firmware/common/$MODULE.cpp" -o "$BUILD_DIR/well_exact_readback/$MODULE.o"
done
PROTECTION_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -std=c++11 -Wall -Wextra -Werror -pedantic -Itests/stubs/protection -Itests/stubs -Ifirmware/common"
for MODULE in NvsWellProtectionStore NvsUpperTankProtectionStore NvsLowerTankPumpProtectionStore NvsUpperTankLimitsStore; do
  $CXX $PROTECTION_FLAGS -c "firmware/common/$MODULE.cpp" -o "$BUILD_DIR/protection/$MODULE.o"
done
$CXX -std=c++11 -Wall -Wextra -Werror -pedantic -Itests/stubs/replay -Itests/stubs -Ifirmware/common -c firmware/common/NvsPeerReplayStore.cpp -o "$BUILD_DIR/replay/NvsPeerReplayStore.o"
$CXX -std=c++11 -Wall -Wextra -Werror -pedantic -Itests/stubs/transfer_journal -Itests/stubs -Ifirmware/common -c firmware/common/NvsTransferJournalStore.cpp -o "$BUILD_DIR/journal/NvsTransferJournalStore.o"

TESTS="test_peer_datagram_envelope test_peer_authenticated_status_receiver test_web_auth_rate_limiter test_credential_random test_local_service_authorization test_credential_store test_credential_powercut test_credential_legacy_migrator test_credential_provisioning_controller test_credential_provisioning_serial_protocol test_credential_provisioning_board_adapter test_credential_startup_gate test_nvs_credential_legacy_migrator test_nvs_credential_text_storage test_nvs_credential_backend test_wifi_sta_credential_store test_hmac_sha256 test_inflow_persistence_policy test_nvs_daily_inflow_store test_nvs_sensor_config_store test_nvs_transfer_history_store test_nvs_lower_tank_inflow_watchdog_store test_nvs_upper_tank_limits_store test_nvs_upper_tank_volume_calibration_store test_nvs_upper_tank_volume_calibration_powercut test_nvs_upper_tank_protection_store test_nvs_well_calibration_store test_nvs_well_geometry_store test_nvs_well_protection_store test_peer_replay_store test_peer_command_frame test_peer_command_guard test_peer_command_response test_peer_command_session test_peer_command_session_persistent test_peer_sequence_allocator test_peer_handshake test_peer_handshake_initiator test_peer_handshake_responder_coordinator test_peer_handshake_retry_policy test_peer_handshake_ready test_peer_handshake_timeout test_timed_peer_handshake_initiator test_timed_peer_handshake_responder_coordinator test_peer_handshake_wire test_peer_protocol test_peer_request_tracker test_peer_secret_vault test_peer_session_codec test_peer_status_session_codec test_peer_status_hmac_codec test_nvs_peer_shared_key_store test_peer_session_keys test_persistent_peer_replay_guard test_sample_freshness test_sensor_config test_sensor_math test_timed_peer_handshake_responder test_transfer_history test_manual_valve_control test_emergency_valve_pump_guard test_transfer_logic test_transfer_restart_guard test_valve_relay_output test_pump_safety_relay_output test_nvs_transfer_journal_store test_upper_tank_volume_calibration test_water_temperature test_upper_tank_limits test_upper_tank_protection test_lower_tank_pump_protection test_nvs_lower_tank_pump_protection_store test_water_math test_well_calibration_editor test_well_geometry test_well_pump_protection test_lower_tank_inflow_watchdog"
COUNT=0
for TEST in $TESTS; do
  if [ "$TEST" = "test_nvs_credential_legacy_migrator" ]; then
    $CXX -Itests/stubs/credential_store_nvs $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_credential_text_storage" ]; then
    $CXX -Itests/stubs/credential_text $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_well_calibration_store" ]; then
    $CXX -Itests/stubs/well_exact_readback $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR/portable/WellGeometry.o" "$BUILD_DIR/portable/WellCalibrationEditor.o" "$BUILD_DIR/portable/WaterMath.o" "$BUILD_DIR/well_exact_readback/NvsWellCalibrationStore.o" -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_well_geometry_store" ]; then
    $CXX -Itests/stubs/well_exact_readback $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR/portable/WellGeometry.o" "$BUILD_DIR/portable/WaterMath.o" "$BUILD_DIR/well_exact_readback/NvsWellGeometryStore.o" -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_transfer_history_store" ]; then
    $CXX -Itests/stubs/history_exact_readback $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR/portable/TransferHistory.o" "$BUILD_DIR/portable/TransferLogic.o" "$BUILD_DIR/portable/WaterMath.o" "$BUILD_DIR/portable/LowerTankInflowWatchdog.o" "$BUILD_DIR/history_exact_readback/NvsTransferHistoryStore.o" -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_peer_replay_store" ]; then
    $CXX -Itests/stubs/replay $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/replay/NvsPeerReplayStore.o "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_transfer_journal_store" ]; then
    $CXX -Itests/stubs/transfer_journal $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/portable/*.o "$BUILD_DIR"/journal/NvsTransferJournalStore.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_well_protection_store" ]; then
    $CXX $PROTECTION_FLAGS "tests/$TEST.cpp" "$BUILD_DIR"/protection/NvsWellProtectionStore.o "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_upper_tank_protection_store" ]; then
    $CXX $PROTECTION_FLAGS "tests/$TEST.cpp" "$BUILD_DIR"/protection/NvsUpperTankProtectionStore.o "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_upper_tank_limits_store" ]; then
    $CXX $PROTECTION_FLAGS "tests/$TEST.cpp" "$BUILD_DIR"/protection/NvsUpperTankLimitsStore.o "$BUILD_DIR/portable/UpperTankLimits.o" -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_lower_tank_pump_protection_store" ]; then
    $CXX $PROTECTION_FLAGS "tests/$TEST.cpp" "$BUILD_DIR"/protection/NvsLowerTankPumpProtectionStore.o "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_valve_relay_output" ]; then
    $CXX $CXXFLAGS -Ifirmware/ESP32_1/src "tests/$TEST.cpp" firmware/ESP32_1/src/ValveRelayOutput.cpp -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_pump_safety_relay_output" ]; then
    $CXX $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR/portable/PumpSafetyRelayOutput.o" -o "$BUILD_DIR/$TEST"
  elif [ "$TEST" = "test_nvs_upper_tank_volume_calibration_powercut" ]; then
    $CXX -Itests/stubs/upper_volume_powercut $CXXFLAGS -c firmware/common/NvsUpperTankVolumeCalibrationStore.cpp -o "$BUILD_DIR/upper_volume_powercut/NvsUpperTankVolumeCalibrationStore.o"
    $CXX -Itests/stubs/upper_volume_powercut $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR/portable/UpperTankVolumeCalibration.o" "$BUILD_DIR/upper_volume_powercut/NvsUpperTankVolumeCalibrationStore.o" -o "$BUILD_DIR/$TEST"
  elif [ "${TEST#test_nvs_}" != "$TEST" ]; then
    $CXX $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/portable/*.o "$BUILD_DIR"/nvs/*.o -o "$BUILD_DIR/$TEST"
  else
    $CXX $CXXFLAGS "tests/$TEST.cpp" "$BUILD_DIR"/portable/*.o -o "$BUILD_DIR/$TEST"
  fi
  "$BUILD_DIR/$TEST"
  COUNT=$((COUNT + 1))
done
printf 'PASS: %s native test programs\n' "$COUNT"

sh "tests/check_peer_boot_identity.sh"
sh "tests/check_peer_boot_epoch_transfer_abort.sh"
sh "tests/check_peer_ethernet_recovery.sh"
sh "tests/check_dashboard_pump_safety_text.sh"
sh "tests/check_credential_module_sync.sh"
sh "tests/check_credential_startup_gate_integration.sh"
sh "tests/check_credential_entropy_integration.sh"
sh "tests/check_wifi_sta_credential_integration.sh"

sh "tests/check_web_auth_rate_limiter_integration.sh"

sh "tests/check_protection_store_sync.sh"
sh "tests/check_nvs_storage_audit.sh"

sh "tests/check_common_module_sync.sh"
sh "tests/check_upper_transfer_valve_gate.sh"

sh "tests/check_manual_valve_control_integration.sh"
sh "tests/check_automatic_transfer_relay_integration.sh"

sh "tests/check_pump_safety_output_gate.sh"
sh "tests/check_pump_protection_independence.sh"

sh "tests/check_lower_tank_pump_protection_integration.sh"
sh "tests/check_lower_tank_working_minimum_informational.sh"

sh "tests/check_peer_status_auth_control_gate.sh"
sh "tests/check_inflow_watchdog_settings_integration.sh"
sh "tests/check_inflow_watchdog_transfer_integration.sh"

sh "tests/check_upper_tank_volume_calibration_integration.sh"
sh "tests/check_upper_temperature_integration.sh"
sh "tests/check_w5500_dashboard_status.sh"

sh "tests/check_upper_tank_limits_store_sync.sh"
