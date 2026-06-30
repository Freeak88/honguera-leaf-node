#!/usr/bin/env python3
"""
Clean Leaf AI modules for Honguera firmware.
Removes all references to: RS485, SDI-12, MCP4725, UARTChain,
WebServer, unused RS485 sensors, EZO probes.
"""
import re, os

BASE = "/root/.openclaw/workspace/projects/fungi/honguera/hardware/leaf-node-fork/firmware/honguera"

def fix_file(path, fixes):
    """Apply regex-based fixes to a file."""
    fpath = os.path.join(BASE, path)
    if not os.path.exists(fpath):
        print(f"  SKIP (not found): {path}")
        return
    with open(fpath) as f: content = f.read()
    original = content
    for pattern, replacement in fixes:
        content = re.sub(pattern, replacement, content, flags=re.MULTILINE)
    if content != original:
        with open(fpath, 'w') as f: f.write(content)
        removed = sum(1 for o, n in zip(original.split('\n'), content.split('\n')) if o != n)
        print(f"  FIXED {path} ({removed} lines changed)")
    else:
        print(f"  OK {path} (no changes needed)")

print("=== 1. SystemManager.h ===")
fix_file("system/SystemManager.h", [
    (r'#include "../network/UARTChainManager.h"\n', ''),
    (r'class MCP4725;\n', ''),
    (r'UARTChainManager\* getChainManager\(\) .*?;\n', '// getChainManager removed (Honguera)\n'),
    (r'bool initializeMCP4725\(\);\n', '// initializeMCP4725 removed (Honguera)\n'),
    (r'MCP4725\* getMCP4725\(\) .*?;\n', 'MCP4725* getMCP4725() { return nullptr; }\n'),
    (r'PWMController\* getPWMControllerMOSFET\(\) .*?;\n', 'PWMController* getPWMControllerMOSFET() { return nullptr; }\n'),
    (r'UARTChainManager chainManager_;\n', '// UARTChainManager chainManager_ removed\n'),
    (r'MCP4725\* mcp4725_;\n', 'MCP4725* mcp4725_ = nullptr;\n'),
    (r'void updateActuatorDACLED\(\);\n', 'void updateActuatorDACLED() {}\n'),
    (r'void incrementActuatorChannel\(\);\n', 'void incrementActuatorChannel() {}\n'),
])

print("\n=== 2. SystemManager.cpp ===")
fix_file("system/SystemManager.cpp", [
    (r'#include "../hardware/MCP4725.h"\n', ''),
    (r'.*chainManager_.*', '// chainManager removed (Honguera)\n'),
    (r'.*initializeMCP4725.*', '// MCP4725 not available on Lite\n'),
    (r'.*mcp4725_\s*=\s*new MCP4725.*', ''),
    (r'.*MCP4725 DAC.*', ''),
    (r'void SystemManager::updateActuatorDACLED\(\)\s*\{.*?\n\}', 'void SystemManager::updateActuatorDACLED() { /* no DAC */ }'),
    (r'void SystemManager::incrementActuatorChannel\(\)\s*\{.*?\n\}', 'void SystemManager::incrementActuatorChannel() { /* no DAC */ }'),
])

print("\n=== 3. CommandHandler.h ===")
fix_file("core/CommandHandler.h", [
    (r'void setMCP4725\(class MCP4725\* mcp4725\).*?;\n', 'void setMCP4725(class MCP4725*) {}\n'),
    (r'class MCP4725\* mcp4725_;\n', 'class MCP4725* mcp4725_ = nullptr;\n'),
    (r'void setPWMControllerMOSFET.*?;\n', 'void setPWMControllerMOSFET(class PWMController*) {}\n'),
])

print("\n=== 4. CommandHandler.cpp ===")
fix_file("core/CommandHandler.cpp", [
    (r'#include "../hardware/MCP4725.h"\n', ''),
    (r'.*MCP4725.*DAC.*', '// MCP4725 removed (Honguera)\n'),
])

print("\n=== 5. SensorManager.h ===")
fix_file("sensors/SensorManager.h", [
    (r'#include "../hardware/RS485Manager.h"\n', ''),
    (r'#include "../hardware/SDI12Manager.h"\n', ''),
    (r'\bRS485Manager\* rs485Manager_;\n', 'void* rs485Manager_ = nullptr;\n'),
    (r'\bSDI12Manager\* sdi12Manager_;\n', 'void* sdi12Manager_ = nullptr;\n'),
    (r'RS485Manager\* rs485Manager,\s+SDI12Manager\* sdi12Manager,', 'void* rs485Manager, void* sdi12Manager,'),
    (r',\s+OneWireManager\* oneWireManager_', ', OneWireManager* oneWireManager_'),
    (r'// RS485 Sensor classes\nclass.*?;\n', ''),
    # Remove RS485 sensor pointer declarations
    (r'class CWTPSS\* cwtpssSensor_;\n', ''),
    (r'class LEAFTHSN\* leafthsnSensor_;\n', ''),
    (r'class CWTSoilTHS\* cwtsoilthsSensor_;\n', ''),
    (r'class CWTTHXXS\* cwtthxxsSensor_;\n', ''),
    (r'class SLT5007\* slt5007Sensor_;\n', ''),
    (r'class TEROS12\* teros12Sensor_;\n', ''),
    (r'class EZOec\* ezoecSensor_;\n', ''),
    (r'class EZOph\* ezophSensor_;\n', ''),
])

print("\n=== 6. SensorManager.cpp ===")
# Simplify: just nullify all RS485/SDI-12 sensor operations
fix_file("sensors/SensorManager.cpp", [
    (r'#include.*RS485Manager.*\n', ''),
    (r'#include.*SDI12Manager.*\n', ''),
    (r'#include.*CWTPSS.*\n', ''),
    (r'#include.*CWTSoilTHS.*\n', ''),
    (r'#include.*CWTTHXXS.*\n', ''),
    (r'#include.*LEAFTHSN.*\n', ''),
    (r'#include.*SLT5007.*\n', ''),
    (r'#include.*TEROS12.*\n', ''),
    (r'#include.*EZOec.*\n', ''),
    (r'#include.*EZOph.*\n', ''),
    (r'RS485Manager\* rs485Manager,\s+SDI12Manager\* sdi12Manager,', 'void* rs485Manager, void* sdi12Manager,'),
    (r'rs485Manager_\(rs485Manager\),\s+sdi12Manager_\(sdi12Manager\),', 'rs485Manager_(nullptr), sdi12Manager_(nullptr),'),
    # Nullify RS485 sensor initializations
    (r'slt5007Sensor_\s*= new SLT5007.*?\n', ''),
    (r'cwtpssSensor_\s*= new CWTPSS.*?\n', ''),
    (r'leafthsnSensor_\s*= new LEAFTHSN.*?\n', ''),
    (r'cwtsoilthsSensor_\s*= new CWTSoilTHS.*?\n', ''),
    (r'teros12Sensor_\s*= new TEROS12.*?\n', ''),
    (r'cwtthxxsSensor_\s*= new CWTTHXXS.*?\n', ''),
    (r'ezoecSensor_\s*= new EZOec.*?\n', ''),
    (r'ezophSensor_\s*= new EZOph.*?\n', ''),
    # Remove sensor reading sections
    (r'if \(slt5007Sensor_.*?\n.*?\n\}\n', ''),
    (r'if \(cwtpssSensor_.*?\n.*?\n\}\n', ''),
    (r'if \(leafthsnSensor_.*?\n.*?\n\}\n', ''),
    (r'if \(cwtsoilthsSensor_.*?\n.*?\n\}\n', ''),
    (r'if \(teros12Sensor_.*?\n.*?\n\}\n', ''),
    (r'if \(cwtthxxsSensor_.*?\n.*?\n\}\n', ''),
    (r'if \(ezoecSensor_.*?\n.*?\n\}\n', ''),
    (r'if \(ezophSensor_.*?\n.*?\n\}\n', ''),
    # Remove bus management sections
    (r'bool needsRS485 = false;.*?//\s*shutdownOneWire', '/* RS485/SDI-12 bus management removed */'),
    (r'if \(needsRS485.*?\n.*?\n\}\n', ''),
    (r'if \(needsSDI12.*?\n.*?\n\}\n', ''),
    (r'// Initialize bus.*?\n.*?\n\}\n', ''),
    (r'// Shutdown.*?\n.*?\n\}\n', ''),
])

print("\n=== 7. ScheduleManager.h ===")
fix_file("system/ScheduleManager.h", [
    (r'MCP4725\* mcp4725_;', 'void* mcp4725_ = nullptr;'),
    (r'MCP4725\* mcp4725,', 'void* mcp4725,'),
])

print("\n=== 8. ScheduleManager.cpp ===")
fix_file("system/ScheduleManager.cpp", [
    (r'#include "../hardware/MCP4725.h"\n', ''),
    (r'MCP4725\* mcp4725', 'void* mcp4725'),
    (r'.*mcp4725_ = mcp4725;', 'mcp4725_ = nullptr; /* no DAC */'),
    (r'.*MCP4725 not available.*', '/* no DAC */'),
    (r'if \(mcp4725_\).*?\n.*?\n\}', 'if (false) {}'),
])

print("\n=== 9. ActuatorStatusPublisher.h ===")
fix_file("network/ActuatorStatusPublisher.h", [
    (r'class MCP4725;', ''),
    (r'MCP4725\* mcp4725_ = nullptr;', 'void* mcp4725_ = nullptr;'),
    (r'MCP4725\* mcp4725 = nullptr', 'void* mcp4725 = nullptr'),
    (r'Logger\* logger_, Actuator\* actuator_, MCP4725\* mcp4725_', 'Logger* logger_, Actuator* actuator_, void* mcp4725_'),
])

print("\n=== 10. ActuatorStatusPublisher.cpp ===")
fix_file("network/ActuatorStatusPublisher.cpp", [
    (r'#include "../hardware/MCP4725.h"\n', ''),
    (r'MCP4725\* mcp4725', 'void* mcp4725'),
    (r'.*mcp4725_ = mcp4725;', 'mcp4725_ = nullptr;'),
])

print("\n=== 11. SerialCommandHandler.cpp ===")
fix_file("diagnostics/SerialCommandHandler.cpp", [
    (r'.*SLT5007.*RS485.*', '// RS485 sensors removed (Honguera)'),
    (r'.*CWTPSS.*RS485.*', '// RS485 sensors removed (Honguera)'),
    (r'.*LEAFTHSN.*RS485.*', ''),
    (r'.*CWTSOILTHS.*RS485.*', ''),
    (r'.*CWTTHXXS.*RS485.*', ''),
])

print("\n=== 12. Remove unused source files ===")
unused = [
    "hardware/MCP4725.cpp", "hardware/MCP4725.h",
    "hardware/RS485.cpp", "hardware/RS485.h",
    "hardware/RS485Manager.cpp", "hardware/RS485Manager.h",
    "hardware/RS485CommandHandler.cpp", "hardware/RS485CommandHandler.h",
    "hardware/SDI12Manager.cpp", "hardware/SDI12Manager.h",
    "hardware/sensors/CWTPSS.cpp", "hardware/sensors/CWTPSS.h",
    "hardware/sensors/CWTSoilTHS.cpp", "hardware/sensors/CWTSoilTHS.h",
    "hardware/sensors/CWTTHXXS.cpp", "hardware/sensors/CWTTHXXS.h",
    "hardware/sensors/EZOec.cpp", "hardware/sensors/EZOec.h",
    "hardware/sensors/EZOph.cpp", "hardware/sensors/EZOph.h",
    "hardware/sensors/LEAFTHSN.cpp", "hardware/sensors/LEAFTHSN.h",
    "hardware/sensors/SLT5007.cpp", "hardware/sensors/SLT5007.h",
    "hardware/sensors/TEROS12.cpp", "hardware/sensors/TEROS12.h",
    "network/UARTChainManager.cpp", "network/UARTChainManager.h",
    "network/WebServerManager.cpp", "network/WebServerManager.h",
]
for u in unused:
    fpath = os.path.join(BASE, u)
    if os.path.exists(fpath):
        os.remove(fpath)
        print(f"  REMOVED {u}")

print("\n✅ Cleanup complete")
