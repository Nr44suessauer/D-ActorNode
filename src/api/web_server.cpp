#include "web_server.h"
#include "device_registry.h"
#include <ArduinoJson.h>
#include "servo_control.h"
#include "button_control.h" // Include for button functionality
#include "advanced_motor.h" // Include for advanced motor control
#include "motor_28byj48.h" // Include for 28BYJ-48 motor control
#include "pin_config.h" // Include for pin configuration
#include "led_control.h" // Include for LED control
#include "driver/gpio.h"
#include <WiFi.h>

// Function declarations
void handleRoot();
void handleColorChange();
void handleHexColorChange();
void handleNotFound();
void handleServoControl(); 
void handleMotorControl(); 
void handleGetButtonState(); // New function declaration for button status
void handleSetButtonEnabled(); // Enable/Disable button
void handleBrightness(); // New function declaration for brightness control
void handleSetHomingMode();     // New function for setting homing mode
void handlePassButton(); // New function declaration for button pass functionality
void handle28BYJ48MotorControl(); // 28BYJ-48 motor control handler
void handle28BYJ48MotorStatus(); // 28BYJ-48 motor status handler
void handlePinConfig(); // Pin configuration handler
void handleSavePinConfig(); // Save pin configuration handler
void handleResetPinConfig(); // Reset pin configuration handler
void handleSystemInfo(); // System info handler
void handleSaveWiFiConfig(); // Save WiFi configuration handler
void handleGetPinConfig(); // Get pin configuration as JSON
void handleSetServoPin(); // Set servo pin
void handleSetLedPin(); // Set LED pin
void handleSetButtonPin(); // Set button pin
void handleGetDeviceInfo(); // Get device information
void handleSetDeviceInfo(); // Set device information

// Device Registry handlers
void handleDeviceList();         // GET  /devices
void handleDeviceAdd();          // POST /devices/add
void handleDeviceRemove();       // GET  /devices/remove?id=N
void handleDeviceInfo();         // GET  /devices/info?id=N
void handleDeviceClear();        // GET  /devices/clear
void handleReboot();             // GET  /reboot

static bool isValidOutputPinForServo(int pin) {
  return pin >= 0 && GPIO_IS_VALID_OUTPUT_GPIO(static_cast<gpio_num_t>(pin));
}


// WebServer configuration
const uint16_t HTTP_PORT = 80;
WebServer server(HTTP_PORT);

// Enhanced HTML interface for motor control
const char html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Advanced Motor Control</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; background: #f4f4f4; }
    h1, h2, h3 { color: #333; }
    .container { max-width: 800px; margin: 0 auto; }
    
    /* Tab-Styling */
    .tab { overflow: hidden; border: 1px solid #ccc; background-color: #f1f1f1; border-radius: 5px 5px 0 0; }
    .tab button { background-color: inherit; float: left; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; }
    .tab button:hover { background-color: #ddd; }
    .tab button.active { background-color: #ccc; }
    .tabcontent { display: none; padding: 20px; border: 1px solid #ccc; border-top: none; background-color: white; border-radius: 0 0 5px 5px; }
    .tabcontent.active { display: block; }
    
    /* Button-Styling */
    .btn-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; margin: 20px 0; }
    .btn { display: block; width: 100%; padding: 15px; border: none; border-radius: 5px; color: white; font-size: 14px; cursor: pointer; transition: 0.3s; }
    .btn:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.2); }
    .btn-primary { background-color: #2196F3; }
    .btn-success { background-color: #4CAF50; }
    .btn-warning { background-color: #FF9800; }
    .btn-danger { background-color: #f44336; }
    .btn-secondary { background-color: #6c757d; }
    
    /* Container-Styling */
    .control-container { margin: 20px 0; padding: 20px; background: #fff; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
    .control-container h3 { margin-top: 0; color: #2196F3; border-bottom: 2px solid #2196F3; padding-bottom: 10px; }
    
    /* Slider-Styling */
    .slider-container { margin: 15px 0; }
    .slider-wrapper { display: flex; align-items: center; justify-content: center; gap: 15px; flex-wrap: wrap; }
    input[type="range"] { flex: 1; min-width: 200px; max-width: 400px; height: 8px; border-radius: 5px; background: #ddd; outline: none; }
    input[type="range"]::-webkit-slider-thumb { appearance: none; width: 20px; height: 20px; border-radius: 50%; background: #2196F3; cursor: pointer; }
    input[type="range"]::-moz-range-thumb { width: 20px; height: 20px; border-radius: 50%; background: #2196F3; cursor: pointer; border: none; }
    
    /* Status Display */
    .status-display { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 20px 0; }
    .status-item { background: #f8f9fa; padding: 15px; border-radius: 8px; border-left: 4px solid #2196F3; }
    .status-label { font-weight: bold; color: #666; margin-bottom: 5px; }
    .status-value { font-size: 18px; color: #333; }

    .pin-conflict {
      border: 1px solid #ddd !important;
      background-color: #fff !important;
      color: inherit !important;
      font-weight: normal;
    }

    .pin-option-occupied {
      color: inherit;
      font-weight: normal;
    }

    .pin-option-unavailable {
      text-decoration: line-through;
      font-style: normal;
    }

    /* Component manager */
    .component-manager { margin: 15px 0 25px 0; padding: 16px; background: #eef6ff; border: 1px solid #cfe6ff; border-radius: 8px; }
    .component-manager-header { display: flex; justify-content: space-between; align-items: center; gap: 10px; flex-wrap: wrap; }
    .component-manager-meta { font-size: 13px; color: #335; }
    .component-list { margin-top: 12px; display: grid; gap: 8px; }
    .component-card { background: #fff; border: 1px solid #d9e7f5; border-radius: 6px; padding: 10px; display: flex; justify-content: space-between; align-items: center; gap: 10px; }
    .component-card-name { font-weight: bold; color: #1f3b57; }
    
    /* Motor-specific Styles */
    .position-input { padding: 10px; font-size: 16px; width: 100px; text-align: center; border: 1px solid #ddd; border-radius: 4px; }
    
    /* LED Control Styles */
    .btn-red { background-color: #f44336; }
    .btn-green { background-color: #4CAF50; }
    .btn-blue { background-color: #2196F3; }
    .btn-yellow { background-color: #FFEB3B; color: black; }
    .btn-purple { background-color: #9C27B0; }
    .btn-orange { background-color: #FF9800; }
    .btn-white { background-color: #FFFFFF; color: black; border: 1px solid #ddd; }
    
    .color-preview { width: 50px; height: 50px; border-radius: 50%; margin: 10px auto; border: 1px solid #ddd; }
    .hex-input { padding: 10px; font-size: 16px; width: 140px; text-align: center; border: 1px solid #ddd; border-radius: 4px; }
    
    /* Toggle Switch Styles */
    .function-row { margin: 15px 0; padding: 15px; background: #f8f9fa; border-radius: 8px; }
    .switch-label { display: flex; align-items: center; gap: 10px; cursor: pointer; }
    .switch-text { font-weight: bold; color: #333; }
    .description { display: block; margin-top: 8px; font-size: 12px; color: #666; font-style: italic; }
    
    .slider-toggle { position: relative; width: 50px; height: 25px; background: #ccc; border-radius: 25px; transition: 0.3s; }
    .slider-toggle:before { content: ""; position: absolute; width: 21px; height: 21px; background: white; border-radius: 50%; top: 2px; left: 2px; transition: 0.3s; }
    input[type="checkbox"] { display: none; }
    input[type="checkbox"]:checked + .slider-toggle { background: #2196F3; }
    input[type="checkbox"]:checked + .slider-toggle:before { transform: translateX(25px); }
    
    /* Collapsible Section */
    .collapsible-header { 
      background: #2196F3; 
      color: white; 
      padding: 15px; 
      cursor: pointer; 
      border: none; 
      width: 100%; 
      text-align: left; 
      font-size: 16px; 
      font-weight: bold; 
      border-radius: 8px; 
      margin-bottom: 10px;
      display: flex;
      align-items: center;
      justify-content: space-between;
    }
    .collapsible-header:hover { background: #1976D2; }
    .collapsible-header.active { background: #1565C0; border-radius: 8px 8px 0 0; margin-bottom: 0; }
    .pinout-toggle { margin-top: 24px; }
    .collapsible-content { 
      max-height: 0; 
      overflow: hidden; 
      transition: max-height 0.3s ease-out; 
      background: #f8f9fa; 
      border-radius: 0 0 8px 8px;
    }
    .collapsible-content.active { 
      max-height: 1000px; 
      padding: 20px; 
      border: 2px solid #2196F3; 
      border-top: none;
    }
    .pinout-image { 
      width: 100%; 
      max-width: 800px; 
      height: auto; 
      display: block; 
      margin: 0 auto;
      border-radius: 8px;
      cursor: zoom-in;
    }

    .pinout-modal {
      display: none;
      position: fixed;
      z-index: 2000;
      inset: 0;
      background: rgba(0, 0, 0, 0.9);
      align-items: center;
      justify-content: center;
      padding: 20px;
      box-sizing: border-box;
    }

    .pinout-modal.active {
      display: flex;
    }

    .pinout-modal-image {
      max-width: 92vw;
      max-height: 86vh;
      width: auto;
      height: auto;
      transform-origin: center center;
      transition: transform 0.08s linear;
      cursor: zoom-out;
      border-radius: 10px;
      box-shadow: 0 10px 40px rgba(0, 0, 0, 0.45);
    }

    .pinout-modal-hint {
      position: absolute;
      top: 12px;
      left: 50%;
      transform: translateX(-50%);
      color: #fff;
      font-size: 13px;
      background: rgba(0, 0, 0, 0.45);
      padding: 6px 10px;
      border-radius: 999px;
    }
    .arrow { 
      transition: transform 0.3s; 
      font-size: 20px;
    }
    .arrow.active { transform: rotate(180deg); }
    
    /* Responsive Design */
    @media (max-width: 768px) {
      .container { padding: 10px; }
      .btn-grid { grid-template-columns: 1fr; }
      .status-display { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔧 ESP32 PositionUnit - Advanced Control</h1>
    
    <!-- Tab Navigation -->
    <div class="tab">
      <button class="tablinks active" onclick="openTab(event, 'MotorTab')">🔩 Motor Control</button>
      <button class="tablinks" onclick="openTab(event, 'Motor28BYJ48Tab')">⚙️ 28BYJ-48 Motor</button>
      <button class="tablinks" onclick="openTab(event, 'ServoTab')">🔄 Servo Control</button>
      <button class="tablinks" onclick="openTab(event, 'LEDTab')">💡 LED Control</button>
      <button class="tablinks" onclick="openTab(event, 'StatusTab')">📊 Status & Info</button>
      <button class="tablinks" onclick="openTab(event, 'InfoTab')">ℹ️ Info</button>
    </div>

    <!-- Motor Control Tab -->
    <div id="MotorTab" class="tabcontent active" data-component="nema23_motor" data-label="NEMA 23 Motor">
      <h2>🔩 Advanced Stepper Motor Control</h2>

      <div class="control-container">
        <h3>Motor Selection</h3>
        <div class="slider-wrapper">
          <label>Motor ID:</label>
          <select id="motorIdSelect" class="position-input" style="width: 140px;" onchange="updateMotorStatus()">
            <option value="1">Motor 1</option>
            <option value="2">Motor 2</option>
            <option value="3">Motor 3</option>
          </select>
        </div>
      </div>
      
      <!-- Motor Status -->
      <div class="control-container">
        <h3>📊 Motor Status</h3>
        <div class="status-display" id="motorStatusDisplay">
          <div class="status-item">
            <div class="status-label">Position</div>
            <div class="status-value" id="currentPosition">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Target Position</div>
            <div class="status-value" id="targetPosition">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Speed</div>
            <div class="status-value" id="currentSpeed">60 RPM</div>
          </div>
          <div class="status-item">
            <div class="status-label">Status</div>
            <div class="status-value" id="motorStatus">Ready</div>
          </div>
        </div>
        <button class="btn btn-secondary" onclick="updateMotorStatus()">Update Status</button>
      </div>

      <!-- Speed Control -->
      <div class="control-container">
        <h3>⚡ Speed Control</h3>
        <div class="slider-wrapper">
          <label>Speed:</label>
          <input type="range" id="speedSlider" min="1" max="500" value="150" oninput="updateSpeedValue(this.value)">
          <span id="speedValue">100</span> RPM
        </div>
      </div>

      <!-- Positioning -->
      <div class="control-container">
        <h3>🎯 Absolute Positioning</h3>
        <div class="slider-wrapper">
          <label>Position:</label>
          <input type="range" id="positionSlider" min="-5000" max="5000" value="0" oninput="updatePositionValue(this.value)">
          <span id="positionValue">0</span> Steps
        </div>
        <div style="margin: 15px 0;">
          <input type="number" id="positionInput" class="position-input" placeholder="Position" value="0">
          <button class="btn btn-primary" onclick="moveToPosition()">Move to Position</button>
        </div>
      </div>

      <!-- Relative Movement -->
      <div class="control-container">
        <h3>➡️ Relative Movement</h3>
        <div class="btn-grid">
          <button class="btn btn-success" onclick="moveRelative(-1000)">⬅️ 1000 Steps</button>
          <button class="btn btn-success" onclick="moveRelative(-100)">⬅️ 100 Steps</button>
          <button class="btn btn-success" onclick="moveRelative(-10)">⬅️ 10 Steps</button>
          <button class="btn btn-success" onclick="moveRelative(10)">➡️ 10 Steps</button>
          <button class="btn btn-success" onclick="moveRelative(100)">➡️ 100 Steps</button>
          <button class="btn btn-success" onclick="moveRelative(1000)">➡️ 1000 Steps</button>
        </div>
      </div>

      <!-- Advanced Functions -->
      <div class="control-container">
        <h3>🔧 Advanced Functions</h3>
        <div class="function-row">
          <span class="description">🏠 Home to Button: Motor homes to physical button</span>
        </div>
        <div class="btn-grid">
          <button class="btn btn-success" onclick="homeToButton()">🏠 Home to Button</button>
        </div>
      </div>

      <div class="control-container">
        <h3>📌 Pin Assignment (NEMA 23)</h3>
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 10px;">
          <div>
            <label>Motor 1 STEP/DIR/EN:</label><br>
            <input type="number" id="cfg_nema_step_1" class="position-input" min="0" max="48" style="width: 70px;" placeholder="STEP">
            <input type="number" id="cfg_nema_dir_1" class="position-input" min="0" max="48" style="width: 70px;" placeholder="DIR">
            <input type="number" id="cfg_nema_en_1" class="position-input" min="-1" max="48" style="width: 70px;" placeholder="EN">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveNEMA23PinsById(1)">💾 Save + Apply M1</button>
          </div>
          <div>
            <label>Motor 2 STEP/DIR/EN:</label><br>
            <input type="number" id="cfg_nema_step_2" class="position-input" min="0" max="48" style="width: 70px;" placeholder="STEP">
            <input type="number" id="cfg_nema_dir_2" class="position-input" min="0" max="48" style="width: 70px;" placeholder="DIR">
            <input type="number" id="cfg_nema_en_2" class="position-input" min="-1" max="48" style="width: 70px;" placeholder="EN">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveNEMA23PinsById(2)">💾 Save + Apply M2</button>
          </div>
          <div>
            <label>Motor 3 STEP/DIR/EN:</label><br>
            <input type="number" id="cfg_nema_step_3" class="position-input" min="0" max="48" style="width: 70px;" placeholder="STEP">
            <input type="number" id="cfg_nema_dir_3" class="position-input" min="0" max="48" style="width: 70px;" placeholder="DIR">
            <input type="number" id="cfg_nema_en_3" class="position-input" min="-1" max="48" style="width: 70px;" placeholder="EN">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveNEMA23PinsById(3)">💾 Save + Apply M3</button>
          </div>
        </div>
        <button class="collapsible-header pinout-toggle" onclick="toggleCollapsible(this)">
          📍 Pinout anzeigen
          <span class="arrow">▼</span>
        </button>
        <div class="collapsible-content">
          <img class="pinout-image" src="https://devboards.info/images/boards/esp32-s3-devkitc-1/esp32-s3-devkitc-1-pinout.webp" alt="ESP32-S3-DevKitC-1 Pinout" loading="lazy" onclick="openPinoutZoom(this.src)">
        </div>
      </div>

      <!-- Button Pass Function -->
      <div class="control-container">
        <h3>🔄 Button Pass Function</h3>
        <div class="function-row">
          <span class="description">🎯 Pass Button Multiple Times: Motor passes the button a specified number of times</span>
        </div>
        <div style="margin: 15px 0; display: flex; align-items: center; gap: 10px; flex-wrap: wrap; justify-content: center;">
          <label>Pass Count:</label>
          <input type="number" id="passCountInput" class="position-input" placeholder="10" value="10" min="1" max="100">
          <button class="btn btn-warning" onclick="passButtonTimes()">🔄 Pass Button</button>
        </div>
        
        <!-- Pass Progress Display -->
        <div class="status-display">
          <div class="status-item">
            <div class="status-label">Target Passes</div>
            <div class="status-value" id="targetPassCount">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Current Passes</div>
            <div class="status-value" id="currentPassCount">0</div>
          </div>
        </div>
      </div>




    </div>

    <!-- 28BYJ-48 Motor Control Tab -->
    <div id="Motor28BYJ48Tab" class="tabcontent" data-component="motor_28byj48" data-label="28BYJ-48 Motor">
      <h2>⚙️ 28BYJ-48 Stepper Motor Control (GPIO 4-7)</h2>
      
      <!-- Motor Status -->
      <div class="control-container">
        <h3>📊 Motor Status</h3>
        <div class="status-display" id="motor28byj48StatusDisplay">
          <div class="status-item">
            <div class="status-label">Position</div>
            <div class="status-value" id="current28byj48Position">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Target Position</div>
            <div class="status-value" id="target28byj48Position">0</div>
          </div>
          <div class="status-item">
            <div class="status-label">Speed</div>
            <div class="status-value" id="current28byj48Speed">50%</div>
          </div>
          <div class="status-item">
            <div class="status-label">Status</div>
            <div class="status-value" id="motor28byj48Status">Ready</div>
          </div>
        </div>
        <button class="btn btn-secondary" onclick="update28BYJ48MotorStatus()">Update Status</button>
      </div>

      <!-- Speed Control -->
      <div class="control-container">
        <h3>⚡ Speed Control</h3>
        <div class="slider-wrapper">
          <label>Speed:</label>
          <input type="range" id="speed28byj48Slider" min="0" max="90" value="50" oninput="update28BYJ48SpeedValue(this.value)">
          <span id="speed28byj48Value">50</span>%
        </div>
      </div>

      <!-- Positioning -->
      <div class="control-container">
        <h3>🎯 Absolute Positioning</h3>
        <div class="slider-wrapper">
          <label>Position:</label>
          <input type="range" id="position28byj48Slider" min="-5000" max="5000" value="0" oninput="update28BYJ48PositionValue(this.value)">
          <span id="position28byj48Value">0</span> Steps
        </div>
        <div style="margin: 15px 0;">
          <input type="number" id="position28byj48Input" class="position-input" placeholder="Position" value="0">
          <button class="btn btn-primary" onclick="move28BYJ48ToPosition()">Move to Position</button>
        </div>
      </div>

      <!-- Relative Movement -->
      <div class="control-container">
        <h3>➡️ Relative Movement</h3>
        <div class="btn-grid">
          <button class="btn btn-success" onclick="move28BYJ48Relative(-1000)">⬅️ 1000 Steps</button>
          <button class="btn btn-success" onclick="move28BYJ48Relative(-100)">⬅️ 100 Steps</button>
          <button class="btn btn-success" onclick="move28BYJ48Relative(-10)">⬅️ 10 Steps</button>
          <button class="btn btn-success" onclick="move28BYJ48Relative(10)">➡️ 10 Steps</button>
          <button class="btn btn-success" onclick="move28BYJ48Relative(100)">➡️ 100 Steps</button>
          <button class="btn btn-success" onclick="move28BYJ48Relative(1000)">➡️ 1000 Steps</button>
        </div>
      </div>

      <!-- Degree Movement -->
      <div class="control-container">
        <h3>🔄 Degree Movement</h3>
        <div style="margin: 15px 0; display: flex; align-items: center; gap: 10px; flex-wrap: wrap; justify-content: center;">
          <label>Degrees:</label>
          <input type="number" id="degrees28byj48Input" class="position-input" placeholder="360" value="360" min="0" max="3600">
          <button class="btn btn-warning" onclick="move28BYJ48Degrees(1)">↻ Clockwise</button>
          <button class="btn btn-warning" onclick="move28BYJ48Degrees(-1)">↺ Counter-Clockwise</button>
        </div>
      </div>

      <!-- Advanced Functions -->
      <div class="control-container">
        <h3>🔧 Advanced Functions</h3>
        <div class="btn-grid">
          <button class="btn btn-success" onclick="home28BYJ48Motor()">🏠 Home Motor</button>
          <button class="btn btn-primary" onclick="calibrate28BYJ48Motor()">⚙️ Calibrate (Set as 0)</button>
          <button class="btn btn-danger" onclick="stop28BYJ48Motor()">⛔ Stop Motor</button>
        </div>
      </div>

      <!-- Pin Configuration -->
      <div class="control-container">
        <h3>📌 Pin Configuration</h3>
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 10px; margin: 15px 0;">
          <div>
            <label>IN1 (GPIO):</label>
            <input type="number" id="pin28byj48_1" class="position-input" value="6" min="0" max="48" style="width: 80px;">
          </div>
          <div>
            <label>IN2 (GPIO):</label>
            <input type="number" id="pin28byj48_2" class="position-input" value="4" min="0" max="48" style="width: 80px;">
          </div>
          <div>
            <label>IN3 (GPIO):</label>
            <input type="number" id="pin28byj48_3" class="position-input" value="7" min="0" max="48" style="width: 80px;">
          </div>
          <div>
            <label>IN4 (GPIO):</label>
            <input type="number" id="pin28byj48_4" class="position-input" value="5" min="0" max="48" style="width: 80px;">
          </div>
        </div>
        <button class="btn btn-warning" onclick="apply28BYJ48PinConfig()">✅ Apply Pin Configuration</button>
        <div style="margin-top: 10px; font-size: 12px; color: #666;">
          Current Pins: <span id="current28byj48Pins">4, 5, 6, 7</span>
        </div>
        <button class="collapsible-header pinout-toggle" onclick="toggleCollapsible(this)">
          📍 Pinout anzeigen
          <span class="arrow">▼</span>
        </button>
        <div class="collapsible-content">
          <img class="pinout-image" src="https://devboards.info/images/boards/esp32-s3-devkitc-1/esp32-s3-devkitc-1-pinout.webp" alt="ESP32-S3-DevKitC-1 Pinout" loading="lazy" onclick="openPinoutZoom(this.src)">
        </div>
      </div>
    </div>

    <!-- Servo Control Tab -->
    <div id="ServoTab" class="tabcontent" data-component="servo" data-label="Servo">
      <h2>🔄 Servo Control</h2>
      
      <div class="control-container">
        <h3>Servo Positioning</h3>
        <div class="slider-wrapper" style="margin-bottom: 12px;">
          <label>Servo ID:</label>
          <select id="servoIdSelect" class="position-input" style="width: 120px;">
            <option value="1">Servo 1</option>
            <option value="2">Servo 2</option>
            <option value="3">Servo 3</option>
          </select>
        </div>
        <div class="slider-wrapper">
          <label>Angle:</label>
          <input type="range" id="servoSlider" min="0" max="180" value="90" oninput="updateServoValue(this.value)">
          <span id="servoValue">90</span>°
        </div>
        <button class="btn btn-primary" onclick="setServoAngle()">Set Position</button>
      </div>
      
      <div class="control-container">
        <h3>Predefined Positions</h3>
        <div class="btn-grid">
          <button class="btn btn-success" onclick="setServoPreset(0)">0° (Left)</button>
          <button class="btn btn-success" onclick="setServoPreset(45)">45°</button>
          <button class="btn btn-success" onclick="setServoPreset(90)">90° (Center)</button>
          <button class="btn btn-success" onclick="setServoPreset(135)">135°</button>
          <button class="btn btn-success" onclick="setServoPreset(180)">180° (Right)</button>
        </div>
      </div>

      <div class="control-container">
        <h3>📌 Pin Assignment (Servo)</h3>
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 10px;">
          <div>
            <label>Servo 1 GPIO:</label>
            <input type="number" id="cfg_servo_p_1" class="position-input" min="0" max="48" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveServoPinById(1)">💾 Save + Apply S1</button>
          </div>
          <div>
            <label>Servo 2 GPIO:</label>
            <input type="number" id="cfg_servo_p_2" class="position-input" min="0" max="48" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveServoPinById(2)">💾 Save + Apply S2</button>
          </div>
          <div>
            <label>Servo 3 GPIO:</label>
            <input type="number" id="cfg_servo_p_3" class="position-input" min="0" max="48" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveServoPinById(3)">💾 Save + Apply S3</button>
          </div>
        </div>
        <button class="collapsible-header pinout-toggle" onclick="toggleCollapsible(this)">
          📍 Pinout anzeigen
          <span class="arrow">▼</span>
        </button>
        <div class="collapsible-content">
          <img class="pinout-image" src="https://devboards.info/images/boards/esp32-s3-devkitc-1/esp32-s3-devkitc-1-pinout.webp" alt="ESP32-S3-DevKitC-1 Pinout" loading="lazy" onclick="openPinoutZoom(this.src)">
        </div>
      </div>
    </div>

    <!-- LED Control Tab -->
    <div id="LEDTab" class="tabcontent" data-component="led" data-label="LED">
      <h2>💡 LED Control</h2>

      <div class="control-container">
        <h3>LED Output Selection</h3>
        <div class="slider-wrapper">
          <label>LED Output:</label>
          <select id="ledIdSelect" class="position-input" style="width: 140px;">
            <option value="1">LED Output 1</option>
            <option value="2">LED Output 2</option>
            <option value="3">LED Output 3</option>
          </select>
        </div>
      </div>
      
      <!-- Brightness -->
      <div class="control-container">
        <h3>Brightness</h3>
        <div class="slider-wrapper">
          <label>Brightness:</label>
          <input type="range" id="brightnessSlider" min="0" max="255" value="5" oninput="updateBrightnessValue(this.value)">
          <span id="brightnessValue">5</span>
        </div>
        <button class="btn btn-primary" onclick="setBrightness()">Set Brightness</button>
      </div>

      <!-- Custom Color -->
      <div class="control-container">
        <h3>Custom Color</h3>
        <div id="colorPreview" class="color-preview"></div>
        <input type="text" id="hexInput" class="hex-input" placeholder="#FF0000" maxlength="7" value="#FF0000"/>
        <button class="btn btn-primary" onclick="changeHexColor()">Set Color</button>
      </div>

      <!-- Predefined Colors -->
      <div class="control-container">
        <h3>Predefined Colors</h3>
        <div class="btn-grid">
          <button class="btn btn-red" onclick="changeColor(0)">Red</button>
          <button class="btn btn-green" onclick="changeColor(1)">Green</button>
          <button class="btn btn-blue" onclick="changeColor(2)">Blue</button>
          <button class="btn btn-yellow" onclick="changeColor(3)">Yellow</button>
          <button class="btn btn-purple" onclick="changeColor(4)">Purple</button>
          <button class="btn btn-orange" onclick="changeColor(5)">Orange</button>
          <button class="btn btn-white" onclick="changeColor(6)">White</button>
        </div>
      </div>

      <div class="control-container">
        <h3>📌 Pin Assignment (LED)</h3>
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); gap: 10px;">
          <div>
            <label>LED 1 GPIO:</label>
            <input type="number" id="cfg_led_p_1" class="position-input" min="0" max="48" style="width: 80px;">
            <label style="margin-left: 10px;">Count:</label>
            <input type="number" id="cfg_led_c_1" class="position-input" min="1" max="300" value="1" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveLedPinById(1)">💾 Save + Apply O1</button>
          </div>
          <div>
            <label>LED 2 GPIO:</label>
            <input type="number" id="cfg_led_p_2" class="position-input" min="0" max="48" style="width: 80px;">
            <label style="margin-left: 10px;">Count:</label>
            <input type="number" id="cfg_led_c_2" class="position-input" min="1" max="300" value="1" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveLedPinById(2)">💾 Save + Apply O2</button>
          </div>
          <div>
            <label>LED 3 GPIO:</label>
            <input type="number" id="cfg_led_p_3" class="position-input" min="0" max="48" style="width: 80px;">
            <label style="margin-left: 10px;">Count:</label>
            <input type="number" id="cfg_led_c_3" class="position-input" min="1" max="300" value="1" style="width: 80px;">
            <button class="btn btn-primary" style="margin-top: 8px;" onclick="saveLedPinById(3)">💾 Save + Apply O3</button>
          </div>
        </div>
        <button class="collapsible-header pinout-toggle" onclick="toggleCollapsible(this)">
          📍 Pinout anzeigen
          <span class="arrow">▼</span>
        </button>
        <div class="collapsible-content">
          <img class="pinout-image" src="https://devboards.info/images/boards/esp32-s3-devkitc-1/esp32-s3-devkitc-1-pinout.webp" alt="ESP32-S3-DevKitC-1 Pinout" loading="lazy" onclick="openPinoutZoom(this.src)">
        </div>
      </div>
    </div>

    <!-- Info Tab -->
    <div id="InfoTab" class="tabcontent" data-component="button" data-label="Button">
      <h2>ℹ️ Info</h2>

      <!-- Button Status -->
      <div class="control-container">
        <h3>Button Status (Pin 45)</h3>
        <div class="status-display">
          <div class="status-item">
            <div class="status-label">Status</div>
            <div class="status-value" id="buttonStatus">Not pressed</div>
          </div>
          <div class="status-item">
            <div class="status-label">Aktiviert</div>
            <div class="status-value" id="buttonEnabledStatus">-</div>
          </div>
        </div>
        <div style="display:flex;gap:12px;align-items:center;justify-content:center;flex-wrap:wrap;margin-top:10px;">
          <button class="btn btn-secondary" onclick="refreshButtonStatus()">Update Status</button>
          <button class="btn btn-primary"   id="btnToggle" onclick="toggleButtonEnabled()">Enable Button</button>
        </div>
      </div>

      <div class="control-container">
        <h3>📌 Pin Assignment (Button)</h3>
        <div style="display: flex; gap: 10px; align-items: center; justify-content: center; flex-wrap: wrap;">
          <label>GPIO Pin:</label>
          <input type="number" id="cfg_btn_p" class="position-input" min="0" max="48" style="width: 80px;">
          <button class="btn btn-primary" onclick="saveButtonPin()">💾 Save + Apply</button>
        </div>
        <button class="collapsible-header pinout-toggle" onclick="toggleCollapsible(this)">
          📍 Pinout anzeigen
          <span class="arrow">▼</span>
        </button>
        <div class="collapsible-content">
          <img class="pinout-image" src="https://devboards.info/images/boards/esp32-s3-devkitc-1/esp32-s3-devkitc-1-pinout.webp" alt="ESP32-S3-DevKitC-1 Pinout" loading="lazy" onclick="openPinoutZoom(this.src)">
        </div>
      </div>
    </div>

    <!-- Status Tab -->
    <div id="StatusTab" class="tabcontent" data-component="system" data-label="System">
      <h2>📊 System Status & Information</h2>

      <!-- System Information -->
      <div class="control-container">
        <h3>📊 System Information</h3>
        <div class="status-display">
          <div class="status-item">
            <div class="status-label">Chip Model</div>
            <div class="status-value" id="chipModel">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">Free Heap</div>
            <div class="status-value" id="freeHeap">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">Uptime</div>
            <div class="status-value" id="uptime">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">WiFi Status</div>
            <div class="status-value" id="wifiConnectionState">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">Active SSID</div>
            <div class="status-value" id="wifiActiveSsid">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">IP Address</div>
            <div class="status-value" id="wifiIpAddress">-</div>
          </div>
          <div class="status-item">
            <div class="status-label">Signal</div>
            <div class="status-value" id="wifiSignal">-</div>
          </div>
        </div>
      </div>

      <div class="control-container">
        <h3>📶 WiFi Configuration (EEPROM saved)</h3>
        <div style="background: #fff; padding: 15px; border-radius: 5px; margin-bottom: 15px;">
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">SSID:</label>
            <input type="text" id="cfg_wifi_ssid" class="hex-input" maxlength="63" style="width: 100%; box-sizing: border-box;" placeholder="Wi-Fi network name">
          </div>
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Passwort:</label>
            <input type="password" id="cfg_wifi_password" class="hex-input" maxlength="63" style="width: 100%; box-sizing: border-box;" placeholder="WLAN-Passwort (leer bei offenem WLAN)">
            <button class="btn btn-secondary" style="margin-top: 8px;" onclick="togglePasswordVisibility()">👁 Passwort anzeigen/ausblenden</button>
          </div>
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Hostname:</label>
            <input type="text" id="cfg_wifi_hostname" class="hex-input" maxlength="31" style="width: 100%; box-sizing: border-box;" placeholder="ESP32-IScan">
          </div>
          <small style="display: block; color: #666; margin-top: 5px;">If the stored Wi-Fi is not reachable, the device automatically attempts the default Wi-Fi on startup.</small>
        </div>
        <div style="display: flex; gap: 10px; justify-content: center; flex-wrap: wrap;">
          <button class="btn btn-primary" onclick="saveWiFiConfig()">💾 Save Wi-Fi + Restart</button>
          <button class="btn btn-secondary" onclick="loadPinConfig()">🔃 Reload Wi-Fi fields</button>
        </div>
      </div>

      <!-- Device Information -->
      <div class="control-container">
        <h3>🏷️ Device Information (EEPROM saved)</h3>
        <div style="background: #fff; padding: 15px; border-radius: 5px; margin-bottom: 15px;">
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Device Name:</label>
            <input type="text" id="cfg_device_name" class="hex-input" maxlength="63" style="width: 100%; box-sizing: border-box;" placeholder="ESP32-IScan">
            <small style="display: block; color: #666; margin-top: 5px;">e.g. I-Scan-PositionUnit-1</small>
          </div>
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Device Number:</label>
            <input type="text" id="cfg_device_number" class="hex-input" maxlength="31" style="width: 100%; box-sizing: border-box;" placeholder="0001">
            <small style="display: block; color: #666; margin-top: 5px;">e.g. 0001, Unit-42, POS-001</small>
          </div>
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Configuration:</label>
            <input type="text" id="cfg_device_config" class="hex-input" maxlength="127" style="width: 100%; box-sizing: border-box;" placeholder="Standard Configuration">
            <small style="display: block; color: #666; margin-top: 5px;">e.g. Lab-Setup-A, Production-Config</small>
          </div>
          <div style="margin: 15px 0;">
            <label style="display: block; font-weight: bold; margin-bottom: 5px;">Description:</label>
            <textarea id="cfg_device_description" class="hex-input" maxlength="255" style="width: 100%; box-sizing: border-box; height: 80px; resize: vertical; padding: 10px;" placeholder="Description of the device and its use"></textarea>
            <small style="display: block; color: #666; margin-top: 5px;">Additional information about this device</small>
          </div>
        </div>
        <div style="display: flex; gap: 10px; justify-content: center;">
          <button class="btn btn-primary" onclick="saveDeviceInfo()">💾 Save Device Information</button>
          <button class="btn btn-secondary" onclick="loadDeviceInfo()">🔃 Aktualisieren</button>
        </div>
        <div id="deviceInfoStatus" style="margin-top: 15px; padding: 10px; border-radius: 5px; display: none;"></div>
      </div>

    </div>

    <!-- Status Display -->
    <div style="position: fixed; bottom: 20px; left: 20px; right: 20px; background: #333; color: white; padding: 10px; border-radius: 5px; z-index: 1000;">
      <span id="status">Status: System ready</span>
    </div>

    <div id="pinoutModal" class="pinout-modal" onclick="closePinoutZoom(event)">
      <div class="pinout-modal-hint">Mouse wheel: Zoom | Click image: Close</div>
      <img id="pinoutModalImage" class="pinout-modal-image" alt="Pinout Zoom" onclick="closePinoutZoom(event)">
    </div>
  </div>
  
  <script>
    // Globale Variablen
    let motorStatusInterval;
    let isPassingActive = false;
    let pinoutZoomLevel = 1;

    // Tab <-> component binding with dynamic instance management
    const componentRegistry = {};
    const componentStorageKey = 'positionunit_component_instances_v1';
    const componentPinFieldMap = {};

    // ESP32-S3-DevKitM-1 pin sets derived from board header pinout.
    // Output-capable pins used for Servo/LED/Motor control in this UI.
    const BOARD_OUTPUT_PINS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 26, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 47, 48];
    const BOARD_INPUT_PINS = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 26, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48];
    const ALL_GPIO_CANDIDATES = Array.from({ length: 49 }, (_, i) => i);
    const RESERVED_OUTPUT_PINS = [0, 19, 20, 45, 46];
    const RESERVED_INPUT_PINS = [0, 46];

    const PIN_SELECTOR_IDS_OUTPUT = [
      'cfg_servo_p_1', 'cfg_servo_p_2', 'cfg_servo_p_3',
      'cfg_led_p_1', 'cfg_led_p_2', 'cfg_led_p_3',
      'cfg_nema_step_1', 'cfg_nema_dir_1', 'cfg_nema_en_1',
      'cfg_nema_step_2', 'cfg_nema_dir_2', 'cfg_nema_en_2',
      'cfg_nema_step_3', 'cfg_nema_dir_3', 'cfg_nema_en_3',
      'pin28byj48_1', 'pin28byj48_2', 'pin28byj48_3', 'pin28byj48_4'
    ];
    const PIN_SELECTOR_IDS_INPUT = ['cfg_btn_p'];
    const PIN_SELECTOR_IDS_WITH_DISABLE = ['cfg_nema_en_1', 'cfg_nema_en_2', 'cfg_nema_en_3'];

    function buildPinSelect(original, pinList, allowDisabled, reservedPins) {
      const select = document.createElement('select');
      select.id = original.id;
      select.className = original.className;
      select.style.cssText = original.style.cssText;
      const allowed = new Set(pinList.map(pin => String(pin)));
      const reserved = new Set((reservedPins || []).map(pin => String(pin)));

      if (allowDisabled) {
        const noneOption = document.createElement('option');
        noneOption.value = '-1';
        noneOption.textContent = 'Disabled (-1)';
        select.appendChild(noneOption);
      }

      ALL_GPIO_CANDIDATES.forEach(pin => {
        const option = document.createElement('option');
        option.value = String(pin);
        option.textContent = 'GPIO ' + pin;

        const isAllowed = allowed.has(option.value) && !reserved.has(option.value);
        if (!isAllowed) {
          option.disabled = true;
          option.classList.add('pin-option-unavailable');
        }

        select.appendChild(option);
      });

      const initial = original.value;
      if (initial !== undefined && initial !== null && initial !== '') {
        const exists = Array.from(select.options).some(opt => opt.value === String(initial));
        if (!exists) {
          const dynamicOption = document.createElement('option');
          dynamicOption.value = String(initial);
          dynamicOption.textContent = 'GPIO ' + initial;
          dynamicOption.classList.add('pin-option-unavailable');
          dynamicOption.disabled = true;
          select.appendChild(dynamicOption);
        }
        select.value = String(initial);
      }

      select.addEventListener('change', updatePinConflictVisualization);
      return select;
    }

    function setupPinSelectors() {
      PIN_SELECTOR_IDS_OUTPUT.forEach(id => {
        const element = document.getElementById(id);
        if (!element || element.tagName === 'SELECT') return;
        const allowDisabled = PIN_SELECTOR_IDS_WITH_DISABLE.includes(id);
        const select = buildPinSelect(element, BOARD_OUTPUT_PINS, allowDisabled, RESERVED_OUTPUT_PINS);
        element.parentNode.replaceChild(select, element);
      });

      PIN_SELECTOR_IDS_INPUT.forEach(id => {
        const element = document.getElementById(id);
        if (!element || element.tagName === 'SELECT') return;
        const select = buildPinSelect(element, BOARD_INPUT_PINS, false, RESERVED_INPUT_PINS);
        element.parentNode.replaceChild(select, element);
      });
    }

    function collectPinSelectors() {
      const ids = [...PIN_SELECTOR_IDS_OUTPUT, ...PIN_SELECTOR_IDS_INPUT];
      return ids
        .map(id => document.getElementById(id))
        .filter(Boolean);
    }

    function updatePinConflictVisualization() {
      const selectors = collectPinSelectors();
      const usage = {};

      selectors.forEach(select => {
        const value = select.value;
        if (value === '' || value === '-1') return;
        usage[value] = (usage[value] || 0) + 1;
      });

      selectors.forEach(select => {
        const value = select.value;
        const isConflict = value !== '' && value !== '-1' && usage[value] > 1;
        select.classList.toggle('pin-conflict', isConflict);

        Array.from(select.options).forEach(option => {
          const ownClaim = (select.value === option.value) ? 1 : 0;
          const occupiedByOthers = option.value !== '' && option.value !== '-1' && ((usage[option.value] || 0) - ownClaim) > 0;
          option.classList.toggle('pin-option-occupied', occupiedByOthers);
        });
      });
    }

    function showStatus(message, isError) {
      const statusEl = document.getElementById('status');
      if (!statusEl) return;
      statusEl.textContent = 'Status: ' + message;
      statusEl.style.color = isError ? '#ffb3b3' : '#ffffff';
    }

    function loadComponentRegistry() {
      try {
        const raw = localStorage.getItem(componentStorageKey);
        return raw ? JSON.parse(raw) : {};
      } catch (error) {
        console.error('Error loading component registry:', error);
        return {};
      }
    }

    function saveComponentRegistry() {
      localStorage.setItem(componentStorageKey, JSON.stringify(componentRegistry));
    }

    function ensureComponentRegistry() {
      const persisted = loadComponentRegistry();
      document.querySelectorAll('.tabcontent[data-component]').forEach(tab => {
        const tabId = tab.id;
        const componentType = tab.dataset.component;
        const componentLabel = tab.dataset.label || componentType;
        const existing = persisted[tabId] || {};

        componentRegistry[tabId] = {
          componentType,
          componentLabel,
          nextId: existing.nextId || 1,
          activeInstanceId: existing.activeInstanceId || null,
          instances: Array.isArray(existing.instances) ? existing.instances : []
        };

        // Ensure active instance is valid
        const hasActive = componentRegistry[tabId].instances.some(i => i.id === componentRegistry[tabId].activeInstanceId);
        if (!hasActive) {
          componentRegistry[tabId].activeInstanceId = componentRegistry[tabId].instances.length > 0
            ? componentRegistry[tabId].instances[0].id
            : null;
        }
      });

      saveComponentRegistry();
    }

    function removeComponentInstance(tabId, instanceId) {
      const state = componentRegistry[tabId];
      if (!state) return;

      state.instances = state.instances.filter(instance => instance.id !== instanceId);
      if (state.activeInstanceId === instanceId) {
        state.activeInstanceId = state.instances.length > 0 ? state.instances[0].id : null;
      }
      saveComponentRegistry();
      renderComponentManager(tabId);
      showStatus(state.componentLabel + ' Instance #' + instanceId + ' removed');
    }

    function setActiveComponentInstance(tabId, instanceId) {
      const state = componentRegistry[tabId];
      if (!state) return;

      const instance = state.instances.find(i => i.id === instanceId);
      if (!instance) return;

      state.activeInstanceId = instanceId;
      saveComponentRegistry();
      renderComponentManager(tabId);
      loadPinsFromActiveInstance(tabId);
      showStatus(state.componentLabel + ' Instance #' + instanceId + ' is active');
    }

    function addComponentInstance(tabId) {
      const state = componentRegistry[tabId];
      if (!state) return;

      const instanceId = state.nextId;
      state.nextId += 1;
      state.instances.push({
        id: instanceId,
        name: state.componentLabel + ' #' + instanceId,
        pins: {},
        createdAt: Date.now()
      });

      if (!state.activeInstanceId) {
        state.activeInstanceId = instanceId;
      }

      saveComponentRegistry();
      renderComponentManager(tabId);
      showStatus(state.componentLabel + ' Instance #' + instanceId + ' created');
    }

    function renderComponentManager(tabId) {
      const tab = document.getElementById(tabId);
      const state = componentRegistry[tabId];
      if (!tab || !state) return;

      // Component manager header/cards are intentionally hidden in all tabs.
      const existing = document.getElementById('componentManager-' + tabId);
      if (existing) existing.remove();
      return;

      let container = document.getElementById('componentManager-' + tabId);
      if (!container) {
        container = document.createElement('div');
        container.id = 'componentManager-' + tabId;
        container.className = 'component-manager';

        const heading = tab.querySelector('h2');
        if (heading && heading.nextSibling) {
          tab.insertBefore(container, heading.nextSibling);
        } else {
          tab.insertBefore(container, tab.firstChild);
        }
      }

      const cards = state.instances.map(instance =>
        '<div class="component-card">' +
          '<div>' +
            '<div class="component-card-name">' + instance.name + '</div>' +
            '<div class="component-manager-meta">ID: ' + instance.id + (state.activeInstanceId === instance.id ? ' | ACTIVE' : '') + '</div>' +
          '</div>' +
          '<div style="display:flex; gap:8px; flex-wrap: wrap;">' +
            '<button class="btn btn-secondary" style="width:auto; padding:8px 12px;" onclick="setActiveComponentInstance(\'' + tabId + '\',' + instance.id + ')">Activate</button>' +
            '<button class="btn btn-danger" style="width:auto; padding:8px 12px;" onclick="removeComponentInstance(\'' + tabId + '\',' + instance.id + ')">Remove</button>' +
          '</div>' +
        '</div>'
      ).join('');

      container.innerHTML =
        '<div class="component-manager-header">' +
          '<div>' +
            '<strong>Component assignment:</strong> ' + state.componentLabel + '<br>' +
            '<span class="component-manager-meta">Tab: ' + tabId + ' | Type: ' + state.componentType + '</span>' +
          '</div>' +
          '<button class="btn btn-primary" style="width:auto;" onclick="addComponentInstance(\'' + tabId + '\')">+ New component</button>' +
        '</div>' +
        '<div class="component-list">' +
          (cards || '<div class="component-manager-meta">No instances created yet.</div>') +
        '</div>';
    }

    function getActiveInstance(tabId) {
      const state = componentRegistry[tabId];
      if (!state || !state.activeInstanceId) return null;
      return state.instances.find(i => i.id === state.activeInstanceId) || null;
    }

    function getPinValuesFromUI(tabId) {
      const fields = componentPinFieldMap[tabId] || [];
      const values = {};
      fields.forEach(fieldId => {
        const el = document.getElementById(fieldId);
        if (el) values[fieldId] = el.value;
      });
      return values;
    }

    function setPinValuesToUI(tabId, values) {
      const fields = componentPinFieldMap[tabId] || [];
      fields.forEach(fieldId => {
        const el = document.getElementById(fieldId);
        if (el && values[fieldId] !== undefined) {
          el.value = values[fieldId];
        }
      });
    }

    function savePinsToActiveInstance(tabId) {
      const state = componentRegistry[tabId];
      const active = getActiveInstance(tabId);
      if (!state || !active) {
        showStatus('Please create and activate an instance first', true);
        return false;
      }

      active.pins = getPinValuesFromUI(tabId);
      saveComponentRegistry();
      showStatus('Pins saved to ' + active.name);
      return true;
    }

    function loadPinsFromActiveInstance(tabId) {
      const active = getActiveInstance(tabId);
      if (!active) {
        showStatus('No active instance available', true);
        return false;
      }

      setPinValuesToUI(tabId, active.pins || {});
      showStatus('Pins loaded from ' + active.name);
      return true;
    }

    function applyPinsFromActiveInstance(tabId) {
      if (!loadPinsFromActiveInstance(tabId)) return;

      if (tabId === 'MotorTab') saveNEMA23Pins();
      if (tabId === 'Motor28BYJ48Tab') apply28BYJ48PinConfig();
      if (tabId === 'ServoTab') saveServoPinForActive();
      if (tabId === 'LEDTab') saveLedPin();
      if (tabId === 'InfoTab') saveButtonPin();
    }

    function getActiveServoIdFromInstance() {
      const active = getActiveInstance('ServoTab');
      if (!active) return 1;
      if (active.id >= 1 && active.id <= 3) {
        return active.id;
      }
      return 1;
    }

    function saveNEMA23PinsForActive() {
      if (!savePinsToActiveInstance('MotorTab')) return;
      saveNEMA23Pins();
    }

    function saveServoPinForActive() {
      if (!savePinsToActiveInstance('ServoTab')) return;
      const servoId = getActiveServoIdFromInstance();
      saveServoPinById(servoId);
    }

    function saveLedPinForActive() {
      if (!savePinsToActiveInstance('LEDTab')) return;
      saveLedPin();
    }

    function saveButtonPinForActive() {
      if (!savePinsToActiveInstance('InfoTab')) return;
      saveButtonPin();
    }

    function initializeComponentManagers() {
      ensureComponentRegistry();
      Object.keys(componentRegistry).forEach(renderComponentManager);
    }

    function syncMissingInstancePinsFromUI() {
      Object.keys(componentRegistry).forEach(tabId => {
        if (!componentPinFieldMap[tabId]) return;
        const active = getActiveInstance(tabId);
        if (!active) return;

        if (!active.pins || Object.keys(active.pins).length === 0) {
          active.pins = getPinValuesFromUI(tabId);
        }
      });
      saveComponentRegistry();
    }
    
    // Tab functionality
    function openTab(evt, tabName) {
      var i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tabcontent");
      for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].classList.remove("active");
      }
      tablinks = document.getElementsByClassName("tablinks");
      for (i = 0; i < tablinks.length; i++) {
        tablinks[i].classList.remove("active");
      }
      document.getElementById(tabName).classList.add("active");
      evt.currentTarget.classList.add("active");
      
      // Update motor status automatically when Motor tab is opened
      if (tabName === 'MotorTab') {
        startMotorStatusUpdates();
      } else {
        stopMotorStatusUpdates();
      }
      
      // Load system and device info when Status tab is opened
      if (tabName === 'StatusTab') {
        loadSystemInfo();
        loadDeviceInfo();
        loadPinConfig();
      }
      
      // Refresh button status when Info tab is opened
      if (tabName === 'InfoTab') {
        refreshButtonStatus();
      }

      // Keep component manager in sync with the active tab
      renderComponentManager(tabName);

      // Pin fields are sourced from device config only (no local profile overwrite).
    }
    
    // Update motor status automatically with dynamic interval
    function startMotorStatusUpdates() {
      updateMotorStatus();
      if (motorStatusInterval) clearInterval(motorStatusInterval);
      // Use faster interval when button-pass is active
      const interval = isPassingActive ? 500 : 2000; // 500ms vs 2000ms
      motorStatusInterval = setInterval(updateMotorStatus, interval);
    }
    
    // Restart updates with new interval
    function restartMotorStatusUpdates() {
      if (motorStatusInterval) {
        startMotorStatusUpdates();
      }
    }
    
    function stopMotorStatusUpdates() {
      if (motorStatusInterval) {
        clearInterval(motorStatusInterval);
        motorStatusInterval = null;
      }
    }
    
    // Page Load Event
    document.addEventListener('DOMContentLoaded', function() {
      initializeComponentManagers();
      setupPinSelectors();
      updateColorPreview();
      refreshButtonStatus();
      updateMotorStatus();
      startMotorStatusUpdates();
      loadPinConfig();
      loadSystemInfo();
      setInterval(loadSystemInfo, 5000); // Update system info every 5 seconds
    });
    
    // Motor Functions
    function updateSpeedValue(val) {
      document.getElementById('speedValue').textContent = val;
    }
    
    function updatePositionValue(val) {
      document.getElementById('positionValue').textContent = val;
      document.getElementById('positionInput').value = val;
    }
    
    function updateMotorStatus() {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      fetch('/motorStatus?motorId=' + motorId)
        .then(response => response.json())
        .then(data => {
          document.getElementById('currentPosition').textContent = data.currentPosition || 0;
          document.getElementById('targetPosition').textContent = data.targetPosition || 0;
          document.getElementById('currentSpeed').textContent = (data.currentSpeed || 60) + ' RPM';
          
          // Pass-Status aktualisieren
          document.getElementById('targetPassCount').textContent = data.targetPassCount || 0;
          document.getElementById('currentPassCount').textContent = data.currentPassCount || 0;
          
          // Check if pass status has changed
          const wasPassingActive = isPassingActive;
          isPassingActive = data.isPassingButton || false;
          
          // Restart updates with new interval if status changed
          if (wasPassingActive !== isPassingActive) {
            restartMotorStatusUpdates();
          }
          
          // Status text based on state
          let statusText = 'Ready';
          
          if (data.isPassingButton) {
              statusText = 'Passing Button (' + (data.currentPassCount || 0) + '/' + (data.targetPassCount || 0) + ')';
          } else if (data.isMoving) {
              statusText = 'Moving';
          } else if (data.isHomed) {
              statusText = 'Ready (Home)';
          }
          
          document.getElementById('motorStatus').textContent = statusText;
        })
        .catch(error => {
          console.error('Error retrieving motor status:', error);
        });
    }
    
    function moveToPosition() {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      const position = parseInt(document.getElementById('positionInput').value) || 0;
      const speed = parseInt(document.getElementById('speedSlider').value) || 60;
      
      document.getElementById('status').innerHTML = 'Status: Motor moving to position ' + position + '...';
      
      fetch('/advancedMotor?motorId=' + motorId + '&action=moveTo&position=' + position + '&speed=' + speed)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          updateMotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in positioning';
        });
    }
    
    function moveRelative(steps) {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      const speed = parseInt(document.getElementById('speedSlider').value); 
      
      document.getElementById('status').innerHTML = 'Status: Motor moving ' + steps + ' steps...';
      
      fetch('/advancedMotor?motorId=' + motorId + '&action=moveRelative&steps=' + steps + '&speed=' + speed)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          updateMotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in relative movement';
        });
    }
    
    // function homeMotor() {
    //   document.getElementById('status').innerHTML = 'Status: Motor moving to home position...';
    //   
    //   // Get speed from speed slider
    //   const speed = document.getElementById('speedSlider').value;
    //   
    //   fetch('/motorHome?speed=' + speed)
    //     .then(response => response.text())
    //     .then(data => {
    //       document.getElementById('status').innerHTML = 'Status: ' + data;
    //       updateMotorStatus();
    //     })
    //     .catch(error => {
    //       document.getElementById('status').innerHTML = 'Status: Error moving to home';
    //     });
    // }
    
    function homeToButton() {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      document.getElementById('status').innerHTML = 'Status: Motor homing to button position...';
      
      // Get speed from speed slider
      const speed = document.getElementById('speedSlider').value;
      
      fetch('/motorHome?motorId=' + motorId + '&speed=' + speed + '&type=button')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          updateMotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error homing to button';
        });
    }
    
    function passButtonTimes() {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      const count = parseInt(document.getElementById('passCountInput').value) || 10;
      const speed = document.getElementById('speedSlider').value;
      
      if (count < 1 || count > 100) {
        document.getElementById('status').innerHTML = 'Status: Pass count must be between 1 and 100';
        return;
      }
      
      document.getElementById('status').innerHTML = 'Status: Motor passing button ' + count + ' times...';
      
      // Immediately enable fast update mode
      isPassingActive = true;
      restartMotorStatusUpdates();
      
      fetch('/passButton?motorId=' + motorId + '&count=' + count + '&speed=' + speed)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          // Force immediate update after completion
          setTimeout(updateMotorStatus, 100);
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error passing button';
          // Reset passing state on error
          isPassingActive = false;
          restartMotorStatusUpdates();
        });
    }
    
    // function calibrateMotor() {
    //   document.getElementById('status').innerHTML = 'Status: Setting virtual home position...';
    //   
    //   fetch('/motorCalibrate')
    //     .then(response => response.text())
    //     .then(data => {
    //       document.getElementById('status').innerHTML = 'Status: ' + data;
    //       updateMotorStatus();
    //     })
    //     .catch(error => {
    //       document.getElementById('status').innerHTML = 'Status: Error setting virtual home';
    //     });
    // }
    

    

    

    
    function stopMotor() {
      const motorId = parseInt(document.getElementById('motorIdSelect').value) || 1;
      document.getElementById('status').innerHTML = 'Status: Motor stopping...';
      
      fetch('/motorStop?motorId=' + motorId)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          updateMotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error stopping motor';
        });
    }
    
    // Homing mode toggle removed - using virtual home only
    

    
    // Servo Functions
    function updateServoValue(val) {
      document.getElementById('servoValue').textContent = val;
    }
    
    function setServoAngle() {
      const servoId = parseInt(document.getElementById('servoIdSelect').value) || 1;
      const angle = document.getElementById('servoSlider').value;
      document.getElementById('status').innerHTML = 'Status: Servo ' + servoId + ' positioning...';
      
      fetch('/setServo?servoId=' + servoId + '&angle=' + angle)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in servo control';
        });
    }
    
    function setServoPreset(angle) {
      document.getElementById('servoSlider').value = angle;
      document.getElementById('servoValue').textContent = angle;
      setServoAngle();
    }
    
    // LED Functions
    function updateBrightnessValue(val) {
      document.getElementById('brightnessValue').textContent = val;
    }
    
    function setBrightness() {
      const brightness = document.getElementById('brightnessSlider').value;
      const ledId = parseInt(document.getElementById('ledIdSelect').value) || 1;
      document.getElementById('status').innerHTML = 'Status: Setting brightness...';
      
      fetch('/setBrightness?ledId=' + ledId + '&value=' + brightness)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error setting brightness';
        });
    }
    
    function updateColorPreview() {
      var hexValue = document.getElementById('hexInput').value;
      if (hexValue.charAt(0) !== '#') {
        hexValue = '#' + hexValue;
        document.getElementById('hexInput').value = hexValue;
      }
      document.getElementById('colorPreview').style.backgroundColor = hexValue;
    }
    
    function changeColor(colorIndex) {
      const ledId = parseInt(document.getElementById('ledIdSelect').value) || 1;
      document.getElementById('status').innerHTML = 'Status: Changing color...';
      fetch('/color?ledId=' + ledId + '&index=' + colorIndex)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error changing color';
        });
    }
    
    function changeHexColor() {
      const ledId = parseInt(document.getElementById('ledIdSelect').value) || 1;
      var hexValue = document.getElementById('hexInput').value;
      if (hexValue.charAt(0) !== '#') {
        hexValue = '#' + hexValue;
      }
      
      document.getElementById('status').innerHTML = 'Status: Changing color...';
      fetch('/hexcolor?ledId=' + ledId + '&hex=' + encodeURIComponent(hexValue))
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error changing color';
        });
    }
    
    // Button Status
    function refreshButtonStatus() {
      fetch('/getButtonState')
        .then(response => response.json())
        .then(data => {
          const buttonStatus = document.getElementById('buttonStatus');
          buttonStatus.textContent = data.pressed ? 'Pressed' : 'Not pressed';
          const enabledEl  = document.getElementById('buttonEnabledStatus');
          const toggleBtn  = document.getElementById('btnToggle');
          if (data.enabled) {
            enabledEl.textContent = 'Yes';
            enabledEl.style.color = 'var(--success, green)';
            toggleBtn.textContent = 'Disable Button';
            toggleBtn.className   = 'btn btn-danger';
          } else {
            enabledEl.textContent = 'No';
            enabledEl.style.color = 'var(--danger, red)';
            toggleBtn.textContent = 'Enable Button';
            toggleBtn.className   = 'btn btn-primary';
          }
        })
        .catch(error => { console.error('Error retrieving button status:', error); });
    }

    function toggleButtonEnabled() {
      const enabledEl = document.getElementById('buttonEnabledStatus');
      const currentlyEnabled = enabledEl.textContent === 'Yes';
      const newVal = currentlyEnabled ? '0' : '1';
      fetch('/setButtonEnabled?enabled=' + newVal)
        .then(r => r.json())
        .then(() => refreshButtonStatus())
        .catch(error => { console.error('Error toggling button:', error); });
    }
    
    // Color preview event listener
    document.addEventListener('DOMContentLoaded', function() {
      const hexInput = document.getElementById('hexInput');
      if (hexInput) {
        hexInput.addEventListener('input', updateColorPreview);
      }
    });
    
    // 28BYJ-48 Motor Functions
    function update28BYJ48SpeedValue(val) {
      document.getElementById('speed28byj48Value').textContent = val;
    }
    
    function update28BYJ48PositionValue(val) {
      document.getElementById('position28byj48Value').textContent = val;
      document.getElementById('position28byj48Input').value = val;
    }
    
    function update28BYJ48MotorStatus() {
      fetch('/motor28byj48Status')
        .then(response => response.json())
        .then(data => {
          document.getElementById('current28byj48Position').textContent = data.currentPosition || 0;
          document.getElementById('target28byj48Position').textContent = data.targetPosition || 0;
          document.getElementById('current28byj48Speed').textContent = (data.currentSpeed || 50) + '%';
          
          // Update pin display and inputs
          if (data.pin1 !== undefined) {
            document.getElementById('current28byj48Pins').textContent = 
              data.pin1 + ', ' + data.pin2 + ', ' + data.pin3 + ', ' + data.pin4;
            document.getElementById('pin28byj48_1').value = data.pin1;
            document.getElementById('pin28byj48_2').value = data.pin2;
            document.getElementById('pin28byj48_3').value = data.pin3;
            document.getElementById('pin28byj48_4').value = data.pin4;
          }
          
          let statusText = 'Ready';
          if (data.isMoving) {
              statusText = 'Moving';
          } else if (data.isHomed) {
              statusText = 'Ready (Homed)';
          }
          
          document.getElementById('motor28byj48Status').textContent = statusText;
        })
        .catch(error => {
          console.error('Error retrieving 28BYJ-48 motor status:', error);
        });
    }
    
    function move28BYJ48ToPosition() {
      const position = parseInt(document.getElementById('position28byj48Input').value) || 0;
      
      document.getElementById('status').innerHTML = 'Status: 28BYJ-48 Motor moving to position ' + position + '...';
      
      fetch('/motor28byj48?action=moveToPosition&position=' + position)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in 28BYJ-48 positioning';
        });
    }
    
    function move28BYJ48Relative(steps) {
      const speed = parseInt(document.getElementById('speed28byj48Slider').value);
      const direction = steps > 0 ? 1 : -1;
      
      document.getElementById('status').innerHTML = 'Status: 28BYJ-48 Motor moving ' + steps + ' steps...';
      
      fetch('/motor28byj48?action=moveRelative&steps=' + Math.abs(steps) + '&direction=' + direction + '&speed=' + speed)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in 28BYJ-48 relative movement';
        });
    }
    
    function move28BYJ48Degrees(direction) {
      const degrees = parseFloat(document.getElementById('degrees28byj48Input').value) || 360;
      const speed = parseInt(document.getElementById('speed28byj48Slider').value);
      
      document.getElementById('status').innerHTML = 'Status: 28BYJ-48 Motor rotating ' + degrees + ' degrees...';
      
      fetch('/motor28byj48?action=moveDegrees&degrees=' + degrees + '&direction=' + direction + '&speed=' + speed)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error in 28BYJ-48 degree movement';
        });
    }
    
    function home28BYJ48Motor() {
      document.getElementById('status').innerHTML = 'Status: 28BYJ-48 Motor moving to home position...';
      
      fetch('/motor28byj48?action=home')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error homing 28BYJ-48 motor';
        });
    }
    
    function calibrate28BYJ48Motor() {
      document.getElementById('status').innerHTML = 'Status: Calibrating 28BYJ-48 Motor...';
      
      fetch('/motor28byj48?action=calibrate')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error calibrating 28BYJ-48 motor';
        });
    }
    
    function stop28BYJ48Motor() {
      document.getElementById('status').innerHTML = 'Status: Stopping 28BYJ-48 Motor...';
      
      fetch('/motor28byj48?action=stop')
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          update28BYJ48MotorStatus();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error stopping 28BYJ-48 motor';
        });
    }
    
    function apply28BYJ48PinConfig() {
      const pin1 = parseInt(document.getElementById('pin28byj48_1').value);
      const pin2 = parseInt(document.getElementById('pin28byj48_2').value);
      const pin3 = parseInt(document.getElementById('pin28byj48_3').value);
      const pin4 = parseInt(document.getElementById('pin28byj48_4').value);
      
      if (pin1 < 0 || pin1 > 48 || pin2 < 0 || pin2 > 48 || 
          pin3 < 0 || pin3 > 48 || pin4 < 0 || pin4 > 48) {
        document.getElementById('status').innerHTML = 'Status: Invalid pin numbers (0-48)';
        return;
      }
      
      document.getElementById('status').innerHTML = 'Status: Applying pin configuration...';
      
      fetch('/motor28byj48?action=setPins&pin1=' + pin1 + '&pin2=' + pin2 + '&pin3=' + pin3 + '&pin4=' + pin4)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          document.getElementById('current28byj48Pins').textContent = pin1 + ', ' + pin2 + ', ' + pin3 + ', ' + pin4;
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error applying pin configuration';
        });
    }
    
    function test28BYJ48CurrentPins() {
      const pin1 = parseInt(document.getElementById('pin28byj48_1').value);
      const pin2 = parseInt(document.getElementById('pin28byj48_2').value);
      const pin3 = parseInt(document.getElementById('pin28byj48_3').value);
      const pin4 = parseInt(document.getElementById('pin28byj48_4').value);
      const testSteps = parseInt(document.getElementById('testSteps28byj48').value) || 50;
      const testDelay = parseInt(document.getElementById('testDelay28byj48').value) || 5;
      
      document.getElementById('status').innerHTML = 'Status: Testing pins ' + pin1 + ', ' + pin2 + ', ' + pin3 + ', ' + pin4 + '...';
      
      fetch('/motor28byj48?action=testPins&pin1=' + pin1 + '&pin2=' + pin2 + '&pin3=' + pin3 + '&pin4=' + pin4 + 
            '&testSteps=' + testSteps + '&testDelay=' + testDelay)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data + ' - Did the motor move?';
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error testing pins';
        });
    }
    
    function confirmPinConfiguration() {
      const pin1 = parseInt(document.getElementById('pin28byj48_1').value);
      const pin2 = parseInt(document.getElementById('pin28byj48_2').value);
      const pin3 = parseInt(document.getElementById('pin28byj48_3').value);
      const pin4 = parseInt(document.getElementById('pin28byj48_4').value);
      
      if (confirm('Confirm pins ' + pin1 + ', ' + pin2 + ', ' + pin3 + ', ' + pin4 + ' as correct configuration?')) {
        apply28BYJ48PinConfig();
      }
    }
    
    // Auto-Test Funktionen
    let autoTestInterval = null;
    let testResults = [];
    let currentTestNumber = 0;
    
    function startAutoTest28BYJ48() {
      const pinList = document.getElementById('autoPinList28byj48').value;
      const testSteps = parseInt(document.getElementById('autoTestSteps28byj48').value) || 50;
      const testDelay = parseInt(document.getElementById('autoTestDelay28byj48').value) || 5;
      const betweenDelay = parseInt(document.getElementById('autoTestBetweenDelay28byj48').value) || 1000;
      const timeout = parseInt(document.getElementById('autoTestTimeout28byj48').value) || 10;
      
      if (!pinList) {
        document.getElementById('status').innerHTML = 'Status: Please enter pin list';
        return;
      }
      
      // Reset results
      testResults = [];
      currentTestNumber = 0;
      document.getElementById('testResultsBody').innerHTML = '';
      
      document.getElementById('status').innerHTML = 'Status: Starting auto-test...';
      document.getElementById('autoTestBtn').disabled = true;
      
      fetch('/motor28byj48?action=autoTest&pins=' + encodeURIComponent(pinList) + 
            '&testSteps=' + testSteps + '&testDelay=' + testDelay + '&betweenDelay=' + betweenDelay +
            '&timeout=' + timeout)
        .then(response => response.text())
        .then(data => {
          document.getElementById('status').innerHTML = 'Status: ' + data;
          // Start polling for feedback requests
          startAutoTestPolling();
        })
        .catch(error => {
          document.getElementById('status').innerHTML = 'Status: Error starting auto-test';
          document.getElementById('autoTestBtn').disabled = false;
        });
    }
    
    function startAutoTestPolling() {
      autoTestInterval = setInterval(checkAutoTestStatus, 500);
    }
    
    function addManualPinCombo() {
      const pin1 = parseInt(document.getElementById('manualPin1').value);
      const pin2 = parseInt(document.getElementById('manualPin2').value);
      const pin3 = parseInt(document.getElementById('manualPin3').value);
      const pin4 = parseInt(document.getElementById('manualPin4').value);
      
      if (isNaN(pin1) || isNaN(pin2) || isNaN(pin3) || isNaN(pin4)) {
        alert('Please enter all pin numbers');
        return;
      }
      
      if (pin1 < 0 || pin1 > 48 || pin2 < 0 || pin2 > 48 || 
          pin3 < 0 || pin3 > 48 || pin4 < 0 || pin4 > 48) {
        alert('Pin numbers must be between 0 and 48');
        return;
      }
      
      addTestResult(pin1, pin2, pin3, pin4, 'Manual');
      document.getElementById('status').innerHTML = 'Status: Combination added to list';
    }
    
    function addManualPinCombo() {
      const pin1 = parseInt(document.getElementById('manualPin1').value);
      const pin2 = parseInt(document.getElementById('manualPin2').value);
      const pin3 = parseInt(document.getElementById('manualPin3').value);
      const pin4 = parseInt(document.getElementById('manualPin4').value);
      
      if (isNaN(pin1) || isNaN(pin2) || isNaN(pin3) || isNaN(pin4)) {
        alert('Please enter all pin numbers');
        return;
      }
      
      if (pin1 < 0 || pin1 > 48 || pin2 < 0 || pin2 > 48 || 
          pin3 < 0 || pin3 > 48 || pin4 < 0 || pin4 > 48) {
        alert('Pin numbers must be between 0 and 48');
        return;
      }
      
      addTestResult(pin1, pin2, pin3, pin4, 'Manual');
      document.getElementById('status').innerHTML = 'Status: Combination added to list';
    }
    
    function addDefaultCombos() {
      clearTestResults();
      addTestResult(6, 4, 7, 5, 'Standard');
      addTestResult(5, 7, 4, 6, 'Reversed');
      addTestResult(6, 7, 4, 5, 'Variant 1');
      addTestResult(5, 4, 7, 6, 'Variant 2');
      addTestResult(4, 6, 5, 7, 'Variant 3');
      addTestResult(7, 5, 6, 4, 'Variant 4');
      document.getElementById('status').innerHTML = 'Status: Default combinations loaded';
    }
    
    function addTestResult(pin1, pin2, pin3, pin4, status) {
      currentTestNumber++;
      testResults.push({num: currentTestNumber, pin1, pin2, pin3, pin4, status});
      
      const tbody = document.getElementById('testResultsBody');
      const row = tbody.insertRow();
      row.style.cursor = 'pointer';
      row.style.borderBottom = '1px solid #eee';
      row.onmouseover = function() { this.style.background = '#f5f5f5'; };
      row.onmouseout = function() { this.style.background = 'white'; };
      
      const statusColor = status === 'Erfolgreich' ? '#4CAF50' : 
                          status === 'Abgelehnt' ? '#f44336' : 
                          status === 'Manuell' ? '#2196F3' : '#ff9800';
      const statusIcon = status === 'Erfolgreich' ? '✓' : 
                         status === 'Abgelehnt' ? '✗' : 
                         status === 'Manuell' ? '✏️' : '⏱';
      
      row.innerHTML = 
        '<td style="padding: 8px;">' + currentTestNumber + '</td>' +
        '<td style="padding: 8px; text-align: center; font-weight: bold;">' + pin1 + '</td>' +
        '<td style="padding: 8px; text-align: center; font-weight: bold;">' + pin2 + '</td>' +
        '<td style="padding: 8px; text-align: center; font-weight: bold;">' + pin3 + '</td>' +
        '<td style="padding: 8px; text-align: center; font-weight: bold;">' + pin4 + '</td>' +
        '<td style="padding: 8px; text-align: center; color: ' + statusColor + ';">' + statusIcon + ' ' + status + '</td>' +
        '<td style="padding: 8px; text-align: center;">' +
        '<button class="btn btn-primary" style="padding: 5px 10px; font-size: 12px;" onclick="selectPinCombination(' + 
        pin1 + ',' + pin2 + ',' + pin3 + ',' + pin4 + ')">Apply</button></td>';
    }
    
    function clearTestResults() {
      testResults = [];
      currentTestNumber = 0;
      document.getElementById('testResultsBody').innerHTML = '';
      document.getElementById('status').innerHTML = 'Status: Results cleared';
    }
    
    function selectPinCombination(pin1, pin2, pin3, pin4) {
      document.getElementById('pin28byj48_1').value = pin1;
      document.getElementById('pin28byj48_2').value = pin2;
      document.getElementById('pin28byj48_3').value = pin3;
      document.getElementById('pin28byj48_4').value = pin4;
      
      if (confirm('Apply and activate pins ' + pin1 + ', ' + pin2 + ', ' + pin3 + ', ' + pin4 + '?')) {
        apply28BYJ48PinConfig();
      }
    }
    
    function checkAutoTestStatus() {
      fetch('/motor28byj48?action=checkAutoTest')
        .then(response => response.json())
        .then(data => {
          if (data.waitingForFeedback) {
            // Zeige Feedback-Dialog
            const currentTestPins = document.getElementById('currentTestPins');
            if (currentTestPins) {
              currentTestPins.textContent = data.pin1 + ', ' + data.pin2 + ', ' + data.pin3 + ', ' + data.pin4;
            }
            const feedback = document.getElementById('autoTestFeedback');
            if (feedback) feedback.style.display = 'block';
            
            // Store current combination for results table
            window.currentTestCombo = {pin1: data.pin1, pin2: data.pin2, pin3: data.pin3, pin4: data.pin4};
          } else if (data.testComplete) {
            // Test abgeschlossen
            clearInterval(autoTestInterval);
            const feedback = document.getElementById('autoTestFeedback');
            if (feedback) feedback.style.display = 'none';
            const autoTestBtn = document.getElementById('autoTestBtn');
            if (autoTestBtn) autoTestBtn.disabled = false;
            
            if (data.success) {
              document.getElementById('status').innerHTML = 
                'Status: ✓ Auto-Test erfolgreich! Pins: ' + 
                data.pin1 + ', ' + data.pin2 + ', ' + data.pin3 + ', ' + data.pin4;
              update28BYJ48MotorStatus();
            } else {
              document.getElementById('status').innerHTML = 
                'Status: Auto-Test abgeschlossen - keine korrekte Kombination gefunden';
            }
          }
        })
        .catch(error => {
          console.error('Error checking auto-test status:', error);
        });
    }
    
    function confirmAutoTest() {
      if (window.currentTestCombo) {
        addTestResult(window.currentTestCombo.pin1, window.currentTestCombo.pin2, 
                      window.currentTestCombo.pin3, window.currentTestCombo.pin4, 'Erfolgreich');
      }
      fetch('/motor28byj48?action=confirmAutoTest')
        .then(response => response.text())
        .then(data => {
          const feedback = document.getElementById('autoTestFeedback');
          if (feedback) feedback.style.display = 'none';
        });
    }
    
    function rejectAutoTest() {
      if (window.currentTestCombo) {
        addTestResult(window.currentTestCombo.pin1, window.currentTestCombo.pin2, 
                      window.currentTestCombo.pin3, window.currentTestCombo.pin4, 'Abgelehnt');
      }
      fetch('/motor28byj48?action=rejectAutoTest')
        .then(response => response.text())
        .then(data => {
          const feedback = document.getElementById('autoTestFeedback');
          if (feedback) feedback.style.display = 'none';
        });
    }
    
    // Keyboard shortcuts for quick feedback
    document.addEventListener('keydown', function(event) {
      const feedback = document.getElementById('autoTestFeedback');
      if (!feedback) return;
      const feedbackVisible = feedback.style.display === 'block';
      if (!feedbackVisible) return;
      
      if (event.key === 'y' || event.key === 'Y' || event.key === 'j' || event.key === 'J') {
        confirmAutoTest();
        event.preventDefault();
      } else if (event.key === 'n' || event.key === 'N') {
        rejectAutoTest();
        event.preventDefault();
      }
    });

    // Pin Configuration Functions
    function setInputValueIfExists(id, value) {
      const el = document.getElementById(id);
      if (el) el.value = value;
    }

    function loadPinConfig() {
      fetch('/pinConfig')
        .then(response => response.json())
        .then(data => {
          // 28BYJ-48 Motor
          setInputValueIfExists('pin28byj48_1', data.motor_28byj48.pin1);
          setInputValueIfExists('pin28byj48_2', data.motor_28byj48.pin2);
          setInputValueIfExists('pin28byj48_3', data.motor_28byj48.pin3);
          setInputValueIfExists('pin28byj48_4', data.motor_28byj48.pin4);
          setInputValueIfExists('cfg_m28_p1', data.motor_28byj48.pin1);
          setInputValueIfExists('cfg_m28_p2', data.motor_28byj48.pin2);
          setInputValueIfExists('cfg_m28_p3', data.motor_28byj48.pin3);
          setInputValueIfExists('cfg_m28_p4', data.motor_28byj48.pin4);
          
          // NEMA 23 Motor
          if (typeof data.nema23 === 'object' && data.nema23 !== null) {
            setInputValueIfExists('cfg_nema_step_1', data.nema23.step1);
            setInputValueIfExists('cfg_nema_dir_1', data.nema23.dir1);
            setInputValueIfExists('cfg_nema_en_1', data.nema23.enable1);
            setInputValueIfExists('cfg_nema_step_2', data.nema23.step2);
            setInputValueIfExists('cfg_nema_dir_2', data.nema23.dir2);
            setInputValueIfExists('cfg_nema_en_2', data.nema23.enable2);
            setInputValueIfExists('cfg_nema_step_3', data.nema23.step3);
            setInputValueIfExists('cfg_nema_dir_3', data.nema23.dir3);
            setInputValueIfExists('cfg_nema_en_3', data.nema23.enable3);
          }
          
          // Servo
          if (typeof data.servo === 'object' && data.servo !== null) {
            setInputValueIfExists('cfg_servo_p_1', data.servo.pin1);
            setInputValueIfExists('cfg_servo_p_2', data.servo.pin2);
            setInputValueIfExists('cfg_servo_p_3', data.servo.pin3);
          } else {
            setInputValueIfExists('cfg_servo_p_1', data.servo);
          }
          
          // LED
          if (typeof data.led === 'object' && data.led !== null) {
            setInputValueIfExists('cfg_led_p_1', data.led.pin1);
            setInputValueIfExists('cfg_led_p_2', data.led.pin2);
            setInputValueIfExists('cfg_led_p_3', data.led.pin3);
            setInputValueIfExists('cfg_led_c_1', data.led.count1);
            setInputValueIfExists('cfg_led_c_2', data.led.count2);
            setInputValueIfExists('cfg_led_c_3', data.led.count3);
          } else {
            setInputValueIfExists('cfg_led_p_1', data.led);
          }
          
          // Button
          setInputValueIfExists('cfg_btn_p', data.button);
          
          // WiFi
          if (data.wifi) {
            setInputValueIfExists('cfg_wifi_ssid', data.wifi.ssid || '');
            setInputValueIfExists('cfg_wifi_password', data.wifi.password || '');
            setInputValueIfExists('cfg_wifi_hostname', data.wifi.hostname || '');
          }

          updatePinConflictVisualization();

          syncMissingInstancePinsFromUI();
          Object.keys(componentRegistry).forEach(renderComponentManager);
        })
        .catch(error => {
          console.error('Error loading pin config:', error);
          showStatus('Error loading pin configuration', true);
        });
    }

    function save28BYJ48Pins() {
      const pin1 = document.getElementById('pin28byj48_1').value;
      const pin2 = document.getElementById('pin28byj48_2').value;
      const pin3 = document.getElementById('pin28byj48_3').value;
      const pin4 = document.getElementById('pin28byj48_4').value;
      
      const formData = new FormData();
      formData.append('component', 'motor_28byj48');
      formData.append('pin1', pin1);
      formData.append('pin2', pin2);
      formData.append('pin3', pin3);
      formData.append('pin4', pin4);
      
      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveNEMA23Pins() {
      const stepPin = document.getElementById('cfg_nema_step_1').value;
      const dirPin = document.getElementById('cfg_nema_dir_1').value;
      const enablePin = document.getElementById('cfg_nema_en_1').value;
      
      const formData = new FormData();
      formData.append('component', 'nema23');
      formData.append('motorId', '1');
      formData.append('stepPin', stepPin);
      formData.append('dirPin', dirPin);
      formData.append('enablePin', enablePin);
      
      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveNEMA23PinsById(motorId) {
      const stepPin = document.getElementById('cfg_nema_step_' + motorId).value;
      const dirPin = document.getElementById('cfg_nema_dir_' + motorId).value;
      const enablePin = document.getElementById('cfg_nema_en_' + motorId).value;

      const formData = new FormData();
      formData.append('component', 'nema23');
      formData.append('motorId', String(motorId));
      formData.append('stepPin', stepPin);
      formData.append('dirPin', dirPin);
      formData.append('enablePin', enablePin);

      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveServoPin() {
      const pin = document.getElementById('cfg_servo_p_1').value;
      
      const formData = new FormData();
      formData.append('component', 'servo');
      formData.append('servoId', '1');
      formData.append('pin', pin);
      
      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveServoPinById(servoId) {
      const field = document.getElementById('cfg_servo_p_' + servoId);
      if (!field) {
        showStatus('Servo pin field not found', true);
        return;
      }

      const pin = field.value;

      const formData = new FormData();
      formData.append('component', 'servo');
      formData.append('servoId', String(servoId));
      formData.append('pin', pin);

      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveLedPin() {
      const pin = document.getElementById('cfg_led_p_1').value;
      const count = document.getElementById('cfg_led_c_1').value;
      
      const formData = new FormData();
      formData.append('component', 'led');
      formData.append('ledId', '1');
      formData.append('pin', pin);
      formData.append('count', count);
      
      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveLedPinById(ledId) {
      const pinField = document.getElementById('cfg_led_p_' + ledId);
      const countField = document.getElementById('cfg_led_c_' + ledId);
      if (!pinField || !countField) {
        showStatus('LED fields not found', true);
        return;
      }

      const pin = pinField.value;
      const count = countField.value;

      const formData = new FormData();
      formData.append('component', 'led');
      formData.append('ledId', String(ledId));
      formData.append('pin', pin);
      formData.append('count', count);

      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function saveButtonPin() {
      const pin = document.getElementById('cfg_btn_p').value;
      
      const formData = new FormData();
      formData.append('component', 'button');
      formData.append('pin', pin);
      
      fetch('/savePinConfig', { method: 'POST', body: formData })
        .then(response => response.text())
        .then(data => {
          showStatus(data);
          loadPinConfig();
        })
        .catch(error => {
          showStatus('Error saving', true);
        });
    }

    function resetPinConfig() {
      if (confirm('Really reset pin configuration to default values?')) {
        fetch('/resetPinConfig')
          .then(response => response.text())
          .then(data => {
            showStatus(data);
            loadPinConfig();
          })
          .catch(error => {
            showStatus('Error resetting', true);
          });
      }
    }

    function togglePasswordVisibility() {
      const passwordField = document.getElementById('cfg_wifi_password');
      passwordField.type = passwordField.type === 'password' ? 'text' : 'password';
    }

    function toggleCollapsible(element) {
      element.classList.toggle('active');
      const content = element.nextElementSibling;
      const arrow = element.querySelector('.arrow');
      
      if (content.classList.contains('active')) {
        content.classList.remove('active');
        arrow.classList.remove('active');
      } else {
        content.classList.add('active');
        arrow.classList.add('active');
      }
    }

    function openPinoutZoom(src) {
      const modal = document.getElementById('pinoutModal');
      const img = document.getElementById('pinoutModalImage');
      if (!modal || !img) return;

      pinoutZoomLevel = 1;
      img.src = src;
      img.style.transform = 'scale(1)';
      modal.classList.add('active');
    }

    function closePinoutZoom(event) {
      if (event) {
        event.stopPropagation();
      }
      const modal = document.getElementById('pinoutModal');
      if (!modal) return;
      modal.classList.remove('active');
    }

    document.addEventListener('wheel', function(event) {
      const modal = document.getElementById('pinoutModal');
      const img = document.getElementById('pinoutModalImage');
      if (!modal || !img || !modal.classList.contains('active')) return;

      event.preventDefault();
      const delta = event.deltaY < 0 ? 0.12 : -0.12;
      pinoutZoomLevel = Math.min(6, Math.max(1, pinoutZoomLevel + delta));
      img.style.transform = 'scale(' + pinoutZoomLevel.toFixed(2) + ')';
    }, { passive: false });

    document.addEventListener('keydown', function(event) {
      if (event.key === 'Escape') {
        closePinoutZoom();
      }
    });

    function saveWiFiConfig() {
      const ssid = document.getElementById('cfg_wifi_ssid').value;
      const password = document.getElementById('cfg_wifi_password').value;
      const hostname = document.getElementById('cfg_wifi_hostname').value;
      
      if (!ssid || ssid.trim() === '') {
        showStatus('SSID must not be empty!', true);
        return;
      }
      
      if (!hostname || hostname.trim() === '') {
        showStatus('Hostname must not be empty!', true);
        return;
      }
      
      if (confirm('The device will reboot and attempt to connect to the new Wi-Fi.\\n\\nSSID: ' + ssid + '\\nHostname: ' + hostname + '\\n\\nContinue?')) {
        const formData = new FormData();
        formData.append('ssid', ssid);
        formData.append('password', password);
        formData.append('hostname', hostname);
        
        fetch('/saveWiFiConfig', { method: 'POST', body: formData })
          .then(response => response.text())
          .then(data => {
            showStatus('Wi-Fi configuration saved. Rebooting in 3 seconds...');
            setTimeout(() => {
              showStatus('Device is rebooting. Please wait ~30 seconds and then connect to the new Wi-Fi.');
            }, 1000);
          })
          .catch(error => {
            showStatus('Error saving', true);
          });
      }
    }

    function loadSystemInfo() {
      fetch('/systemInfo')
        .then(response => response.json())
        .then(data => {
          document.getElementById('chipModel').textContent = data.chipModel + ' (' + data.chipCores + ' cores)';
          document.getElementById('freeHeap').textContent = (data.freeHeap / 1024).toFixed(1) + ' KB / ' + (data.heapSize / 1024).toFixed(1) + ' KB';
          
          const uptimeSeconds = data.uptime;
          const hours = Math.floor(uptimeSeconds / 3600);
          const minutes = Math.floor((uptimeSeconds % 3600) / 60);
          const seconds = uptimeSeconds % 60;
          document.getElementById('uptime').textContent = hours + 'h ' + minutes + 'm ' + seconds + 's';

          const wifiConnected = !!data.wifiConnected;
          const wifiStateElement = document.getElementById('wifiConnectionState');
          const wifiSsidElement = document.getElementById('wifiActiveSsid');
          const wifiIpElement = document.getElementById('wifiIpAddress');
          const wifiSignalElement = document.getElementById('wifiSignal');

          if (wifiStateElement) {
            wifiStateElement.textContent = wifiConnected ? 'Connected' : 'Not connected';
          }
          if (wifiSsidElement) {
            wifiSsidElement.textContent = wifiConnected ? (data.wifiSsid || '-') : '-';
          }
          if (wifiIpElement) {
            wifiIpElement.textContent = wifiConnected ? (data.wifiIp || '-') : '-';
          }
          if (wifiSignalElement) {
            wifiSignalElement.textContent = wifiConnected ? String(data.wifiRssi) + ' dBm' : '-';
          }
        })
        .catch(error => {
          console.error('Error loading system info:', error);
        });
    }

    // Device Information functions
    function loadDeviceInfo() {
      fetch('/getDeviceInfo')
        .then(response => response.json())
        .then(data => {
          document.getElementById('cfg_device_name').value = data.deviceName || '';
          document.getElementById('cfg_device_number').value = data.deviceNumber || '';
          document.getElementById('cfg_device_config').value = data.configuration || '';
          document.getElementById('cfg_device_description').value = data.description || '';
          showDeviceInfoStatus('Device Information loaded', false);
        })
        .catch(error => {
          console.error('Error loading device info:', error);
          showDeviceInfoStatus('Error loading device information', true);
        });
    }

    function saveDeviceInfo() {
      const deviceName = document.getElementById('cfg_device_name').value;
      const deviceNumber = document.getElementById('cfg_device_number').value;
      const configuration = document.getElementById('cfg_device_config').value;
      const description = document.getElementById('cfg_device_description').value;

      let url = '/setDeviceInfo?';
      const params = [];
      
      if (deviceName) params.push('deviceName=' + encodeURIComponent(deviceName));
      if (deviceNumber) params.push('deviceNumber=' + encodeURIComponent(deviceNumber));
      if (configuration) params.push('configuration=' + encodeURIComponent(configuration));
      if (description) params.push('description=' + encodeURIComponent(description));
      
      if (params.length === 0) {
        showDeviceInfoStatus('No changes to save', true);
        return;
      }
      
      url += params.join('&');
      
      fetch(url)
        .then(response => response.text())
        .then(data => {
          showDeviceInfoStatus('✅ ' + data, false);
          // Update after saving
          setTimeout(() => loadDeviceInfo(), 500);
        })
        .catch(error => {
          console.error('Error saving device info:', error);
          showDeviceInfoStatus('❌ Error saving', true);
        });
    }

    function showDeviceInfoStatus(message, isError) {
      const statusElement = document.getElementById('deviceInfoStatus');
      statusElement.textContent = message;
      statusElement.style.display = 'block';
      statusElement.style.backgroundColor = isError ? '#ffebee' : '#e8f5e9';
      statusElement.style.color = isError ? '#c62828' : '#2e7d32';
      statusElement.style.border = '1px solid ' + (isError ? '#ef9a9a' : '#a5d6a7');
      
      // Verstecke Status nach 5 Sekunden
      setTimeout(() => {
        statusElement.style.display = 'none';
      }, 5000);
    }
  </script>
</body>
</html>
)rawliteral";

void setupWebServer() {
  // Set up web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/color", HTTP_GET, handleColorChange);
  server.on("/hexcolor", HTTP_GET, handleHexColorChange); 
  server.on("/setServo", HTTP_GET, handleServoControl);
  server.on("/setMotor", HTTP_GET, handleMotorControl);
  server.on("/getButtonState", HTTP_GET, handleGetButtonState); // Button status handler
  server.on("/setButtonEnabled", HTTP_GET, handleSetButtonEnabled); // Enable/Disable button
  server.on("/setBrightness", HTTP_GET, handleBrightness); // New brightness handler
  
  // Advanced Motor Routes
  server.on("/advancedMotor", HTTP_GET, handleAdvancedMotorControl);
  server.on("/motorStatus", HTTP_GET, handleAdvancedMotorStatus);
  server.on("/motorStop", HTTP_GET, handleAdvancedMotorStop);
  server.on("/motorHome", HTTP_GET, handleAdvancedMotorHome);
  server.on("/motorJog", HTTP_GET, handleAdvancedMotorJog);
  server.on("/motorCalibrate", HTTP_GET, handleAdvancedMotorCalibrate);
  server.on("/setHomingMode", HTTP_GET, handleSetHomingMode);               // New route for homing mode
  server.on("/passButton", HTTP_GET, handlePassButton);                      // New route for button passes

  // 28BYJ-48 Motor Routes
  server.on("/motor28byj48", HTTP_GET, handle28BYJ48MotorControl);
  server.on("/motor28byj48Status", HTTP_GET, handle28BYJ48MotorStatus);

  // Pin Configuration Routes
  server.on("/pinConfig", HTTP_GET, handlePinConfig);
  server.on("/savePinConfig", HTTP_POST, handleSavePinConfig);
  server.on("/resetPinConfig", HTTP_GET, handleResetPinConfig);
  server.on("/systemInfo", HTTP_GET, handleSystemInfo);
  server.on("/saveWiFiConfig", HTTP_POST, handleSaveWiFiConfig);
  server.on("/getPinConfig", HTTP_GET, handleGetPinConfig);
  server.on("/setServoPin", HTTP_GET, handleSetServoPin);
  server.on("/setLedPin", HTTP_GET, handleSetLedPin);
  server.on("/setButtonPin", HTTP_GET, handleSetButtonPin);
  server.on("/getDeviceInfo", HTTP_GET, handleGetDeviceInfo);
  server.on("/setDeviceInfo", HTTP_GET, handleSetDeviceInfo);

  // Device Registry
  server.on("/devices",        HTTP_GET,  handleDeviceList);
  server.on("/devices/add",    HTTP_POST, handleDeviceAdd);
  server.on("/devices/remove", HTTP_GET,  handleDeviceRemove);
  server.on("/devices/info",   HTTP_GET,  handleDeviceInfo);
  server.on("/devices/clear",  HTTP_GET,  handleDeviceClear);

  // Per-type shortcut POSTs
  server.on("/buttons",  HTTP_POST, handleDeviceAdd);
  server.on("/leds",     HTTP_POST, handleDeviceAdd);
  server.on("/motors",   HTTP_POST, handleDeviceAdd);
  server.on("/servos",   HTTP_POST, handleDeviceAdd);
  server.on("/steppers", HTTP_POST, handleDeviceAdd);

  // System
  server.on("/reboot",         HTTP_GET,  handleReboot);

  
  server.onNotFound(handleNotFound);

  // Start web server
  server.begin();
  Serial.println("HTTP server started on port " + String(HTTP_PORT));
}

// Handle incoming client requests
void handleWebServerRequests() {
  server.handleClient();
}

// ─── Device Registry Handlers ───────────────────────────────────────────────

// GET /devices  – all registered devices as JSON
void handleDeviceList() {
  server.send(200, "application/json", getDeviceListJson());
}

// GET /devices/info?id=N
void handleDeviceInfo() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"missing id\"}");
    return;
  }
  uint16_t id = (uint16_t)server.arg("id").toInt();
  const DeviceEntry* e = getDeviceById(id);
  if (!e) {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
    return;
  }
  String json = "{\"id\":"     + String(e->id) +
                ",\"type\":\"" + deviceTypeToString(e->type) + "\"" +
                ",\"name\":\"" + String(e->name) + "\"" +
                ",\"config\":" + String(e->config) + "}";
  server.send(200, "application/json", json);
}

// GET /devices/remove?id=N
void handleDeviceRemove() {
  if (!server.hasArg("id")) {
    server.send(400, "application/json", "{\"error\":\"missing id\"}");
    return;
  }
  uint16_t id = (uint16_t)server.arg("id").toInt();
  if (removeDevice(id)) {
    server.send(200, "application/json", "{\"ok\":true,\"removed\":" + String(id) + "}");
  } else {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
  }
}

// POST /devices/add  (oder /buttons, /leds, /motors, /servos, /steppers)
// Body JSON: {"type":"button","name":"MyBtn","pin":45,"pullup":true}
// Bei Verwendung der Typ-Pfade (/buttons etc.) ist "type" optional.
void handleDeviceAdd() {
  // Typ aus Pfad ableiten wenn kein "type" im Body
  String path = server.uri();
  DeviceType pathType = DEV_UNKNOWN;
  if      (path == "/buttons")  pathType = DEV_BUTTON;
  else if (path == "/leds")     pathType = DEV_LED;
  else if (path == "/motors")   pathType = DEV_MOTOR;
  else if (path == "/servos")   pathType = DEV_SERVO;
  else if (path == "/steppers") pathType = DEV_STEPPER;

  String body = server.arg("plain");
  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"empty body\"}");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, body) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"invalid JSON\"}");
    return;
  }
  // Typ bestimmen
  DeviceType type = pathType;
  if (type == DEV_UNKNOWN && doc["type"].is<const char*>()) {
    type = deviceTypeFromString(doc["type"] | "");
  }
  if (type == DEV_UNKNOWN) {
    server.send(400, "application/json", "{\"error\":\"type missing or unknown\"}");
    return;
  }

  // Name – in String kopieren BEVOR doc modifiziert wird
  String name = doc["name"] | "unnamed";

  // Config-Objekt extrahieren (alles ausser type & name)
  doc.remove("type");
  doc.remove("name");
  String configJson;
  serializeJson(doc, configJson);

  int newId = addDevice(type, name.c_str(), configJson.c_str());
  if (newId < 0) {
    server.send(507, "application/json", "{\"error\":\"registry full\"}");
    return;
  }

  server.send(201, "application/json",
    "{\"ok\":true,\"id\":" + String(newId) +
    ",\"type\":\"" + deviceTypeToString(type) + "\"" +
    ",\"name\":\"" + String(name) + "\"" +
    ",\"note\":\"Hardware active after next restart\"}");
}

// Clear all device registry entries
void handleDeviceClear() {
  clearDeviceRegistry();
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Registry cleared\"}");
}

// Reboot ESP
void handleReboot() {
  server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Rebooting...\"}" );
  delay(500);
  ESP.restart();
}

// Handle root route
void handleRoot() {
  // Use PROGMEM string directly with send_P
  server.send_P(200, "text/html", html);
}

// Handle color change request with predefined colors
void handleColorChange() {
  if (server.hasArg("index")) {
    int ledId = server.hasArg("ledId") ? server.arg("ledId").toInt() : 1;
    int colorIndex = server.arg("index").toInt();
    if (!setColorByIndexForLed(ledId, colorIndex)) {
      server.send(400, "text/plain", "Invalid ledId (1..3)");
      return;
    }
    server.send(200, "text/plain", "LED output " + String(ledId) + " color changed to index " + String(colorIndex));
  } else {
    server.send(400, "text/plain", "Missing 'index' parameter");
  }
}

// Handle hex color change request
void handleHexColorChange() {
  if (server.hasArg("hex")) {
    int ledId = server.hasArg("ledId") ? server.arg("ledId").toInt() : 1;
    String hexColor = server.arg("hex");
    
    // Remove # if present
    if (hexColor.startsWith("#")) {
      hexColor = hexColor.substring(1);
    }
    
    // Parse hex values
    uint32_t rgbColor = strtoul(hexColor.c_str(), NULL, 16);
    int r = (rgbColor >> 16) & 0xFF;
    int g = (rgbColor >> 8) & 0xFF;
    int b = rgbColor & 0xFF;
    
    // Set color
    if (!setColorRGBForLed(ledId, r, g, b)) {
      server.send(400, "text/plain", "Invalid ledId (1..3)");
      return;
    }
    
    server.send(200, "text/plain", "LED output " + String(ledId) + " color changed to #" + hexColor);
  } else {
    server.send(400, "text/plain", "Missing 'hex' parameter");
  }
}

// Handle servo control request
void handleServoControl() {
  if (server.hasArg("angle")) {
    int servoId = server.hasArg("servoId") ? server.arg("servoId").toInt() : 1;
    int angle = server.arg("angle").toInt();
    if (!setServoAngleById(servoId, angle)) {
      server.send(400, "text/plain", "Invalid servoId (1..3)");
      return;
    }
    server.send(200, "text/plain", "Servo " + String(servoId) + " positioned to " + String(angle) + " degrees");
  } else {
    server.send(400, "text/plain", "Missing 'angle' parameter");
  }
}

// Handle motor control request (legacy)
void handleMotorControl() {
  // For absolute position
  if (server.hasArg("position")) {
    int position = server.arg("position").toInt();
    move28BYJ48MotorToPosition(position);
    server.send(200, "text/plain", "Motor positioned to " + String(position));
    return;
  }
  
  // For stepping by number of steps
  if (server.hasArg("steps") && server.hasArg("direction")) {
    int steps = server.arg("steps").toInt();
    int direction = server.arg("direction").toInt();
    
    int speed = 100;
    if (server.hasArg("speed")) {
      speed = server.arg("speed").toInt();
    }
    
    move28BYJ48MotorWithSpeed(steps, direction, speed);
    
    server.send(200, "text/plain", "Motor moved " + String(steps) + 
                " steps in direction " + String(direction) + 
                " with speed " + String(speed) + "%");
    return;
  }
  
  server.send(400, "text/plain", "Missing or invalid parameters");
}

// Handle button state request
void handleGetButtonState() {
  bool isButtonPressed = getButtonState();
  String jsonResponse = "{\"pressed\":" + String(isButtonPressed ? "true" : "false") +
                        ",\"enabled\":" + String(isButtonEnabled() ? "true" : "false") + "}";
  server.send(200, "application/json", jsonResponse);
}

void handleSetButtonEnabled() {
  if (!server.hasArg("enabled")) {
    server.send(400, "text/plain", "Missing 'enabled' parameter (0 or 1)");
    return;
  }
  bool enable = server.arg("enabled") == "1" || server.arg("enabled").equalsIgnoreCase("true");
  setButtonEnabled(enable);
  server.send(200, "application/json",
    "{\"enabled\":" + String(enable ? "true" : "false") + "}");
}

// Handle LED brightness control
void handleBrightness() {
  if (server.hasArg("value")) {
    int ledId = server.hasArg("ledId") ? server.arg("ledId").toInt() : 1;
    int brightness = server.arg("value").toInt();
    brightness = constrain(brightness, 0, 255);
    if (!setBrightnessForLed(ledId, brightness)) {
      server.send(400, "text/plain", "Invalid ledId (1..3)");
      return;
    }
    server.send(200, "text/plain", "LED output " + String(ledId) + " brightness set to " + String(brightness));
  } else {
    server.send(400, "text/plain", "Missing 'value' parameter");
  }
}

// Advanced Motor Handler
void handleAdvancedMotorControl() {
  if (!server.hasArg("action")) {
    server.send(400, "text/plain", "Missing 'action' parameter");
    return;
  }
  
  uint8_t motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
  AdvancedStepperMotor& motor = getAdvancedMotorById(motorId);
  String action = server.arg("action");
  
  if (action == "moveTo" && server.hasArg("position")) {
    int position = server.arg("position").toInt();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 60;
    
    motor.setSpeed(speed);
    motor.moveTo(position);
    
    server.send(200, "text/plain", "Motor " + String(motorId) + " moved to position " + String(position));
    
  } else if (action == "moveRelative" && server.hasArg("steps")) {
    int steps = server.arg("steps").toInt();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 60;
    
    motor.setSpeed(speed);
    motor.moveRelative(steps);
    
    server.send(200, "text/plain", "Motor " + String(motorId) + " moved " + String(steps) + " steps");
    
  } else if (action == "setHome") {
    motor.setHome();
    server.send(200, "text/plain", "Home position set");
    server.send(200, "text/plain", "Emergency stop executed");
    
  } else {
    server.send(400, "text/plain", "Invalid action or missing parameters");
  }
}

void handleAdvancedMotorStatus() {
  uint8_t motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
  AdvancedMotorStatus status = getAdvancedMotorById(motorId).getStatus();
  
  String jsonResponse = "{"
    "\"currentPosition\":" + String(status.currentPosition) + ","
    "\"targetPosition\":" + String(status.targetPosition) + ","
    "\"isMoving\":" + String(status.isMoving ? "true" : "false") + ","
    "\"currentSpeed\":" + String(status.currentSpeed) + ","
    "\"isHomed\":" + String(status.isHomed ? "true" : "false") + ","
    "\"isEnabled\":" + String(status.isEnabled ? "true" : "false") + ","
    "\"targetPassCount\":" + String(status.targetPassCount) + ","
    "\"currentPassCount\":" + String(status.currentPassCount) + ","
    "\"isPassingButton\":" + String(status.isPassingButton ? "true" : "false") +
    "}";
  
  server.send(200, "application/json", jsonResponse);
}

void handleAdvancedMotorStop() {
  uint8_t motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
  getAdvancedMotorById(motorId).stop();
  server.send(200, "text/plain", "Motor " + String(motorId) + " stopped");
}

void handleAdvancedMotorHome() {
  uint8_t motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
  AdvancedStepperMotor& motor = getAdvancedMotorById(motorId);
  // Geschwindigkeit setzen (gleiche Logik wie moveRelative)
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 60;
  
  // Check if button-home is requested
  String homeType = server.hasArg("type") ? server.arg("type") : "position";
  
  Serial.println("Home mit Speed: " + String(speed) + " RPM, Type: " + homeType);
  
  motor.setSpeed(speed);
  
  if (homeType == "button") {
    // Neue Button-basierte Home-Fahrt
    Serial.println("Starte Button-basierte Home-Fahrt");
    motor.homeToButton();
    server.send(200, "text/plain", "Motor " + String(motorId) + " homed to button position");
  } else {
    // Alte Methode: Vereinfachte Home-Position: Fahre zu Position 0
    int currentPos = motor.getCurrentPosition();
    int stepsToHome = -currentPos;  // Anzahl Schritte zu Position 0
    Serial.println("Fahre zur Home-Position (0): " + String(stepsToHome) + " Schritte");
    motor.moveRelative(stepsToHome);
    motor.setHome();  // Position als Home markieren
    server.send(200, "text/plain", "Motor " + String(motorId) + " moved to home position");
  }
}

void handleAdvancedMotorJog() {
  server.send(400, "text/plain", "Jog function removed - use relative movement instead");
}

void handleAdvancedMotorCalibrate() {
  server.send(400, "text/plain", "Calibration function removed");
}

void handleSetHomingMode() {
  // Homing mode functionality removed - using simple virtual home only
  server.send(200, "text/plain", "Homing mode set to Virtual Home (Position 0)");
}

void handlePassButton() {
  uint8_t motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
  AdvancedStepperMotor& motor = getAdvancedMotorById(motorId);
  // Check if count parameter is present
  if (!server.hasArg("count")) {
    server.send(400, "text/plain", "Missing count parameter");
    return;
  }
  
  int count = server.arg("count").toInt();
  int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 60;
  
  // Validierung
  if (count <= 0) {
    server.send(400, "text/plain", "Count must be greater than 0");
    return;
  }
  
  if (count > 100) {
    server.send(400, "text/plain", "Count must be 100 or less for safety");
    return;
  }
  
  Serial.println("Button-Pass-Fahrt mit Count: " + String(count) + ", Speed: " + String(speed) + " RPM");
  
  // Geschwindigkeit setzen und Button-Pass-Fahrt starten
  motor.setSpeed(speed);
  motor.passButtonTimes(count);
  server.send(200, "text/plain", "Motor " + String(motorId) + " pass button command executed");
}

// 28BYJ-48 Motor Control Handler
void handle28BYJ48MotorControl() {
  if (!server.hasArg("action")) {
    server.send(400, "text/plain", "Missing action parameter");
    return;
  }
  
  String action = server.arg("action");
  
  if (action == "setPins" && server.hasArg("pin1") && server.hasArg("pin2") && 
      server.hasArg("pin3") && server.hasArg("pin4")) {
    int pin1 = server.arg("pin1").toInt();
    int pin2 = server.arg("pin2").toInt();
    int pin3 = server.arg("pin3").toInt();
    int pin4 = server.arg("pin4").toInt();
    
    Serial.printf("28BYJ-48: Setting pins to %d, %d, %d, %d (saving to EEPROM)\n", pin1, pin2, pin3, pin4);
    set28BYJ48Pins(pin1, pin2, pin3, pin4);
    setup28BYJ48Motor();
    
    server.send(200, "text/plain", "Pins configured and saved to EEPROM: " + String(pin1) + ", " + 
                String(pin2) + ", " + String(pin3) + ", " + String(pin4));
    
  } else if (action == "testPins" && server.hasArg("pin1") && server.hasArg("pin2") && 
             server.hasArg("pin3") && server.hasArg("pin4")) {
    int pin1 = server.arg("pin1").toInt();
    int pin2 = server.arg("pin2").toInt();
    int pin3 = server.arg("pin3").toInt();
    int pin4 = server.arg("pin4").toInt();
    int testSteps = server.hasArg("testSteps") ? server.arg("testSteps").toInt() : 50;
    int testDelay = server.hasArg("testDelay") ? server.arg("testDelay").toInt() : 5;
    
    Serial.printf("28BYJ-48: Testing pin combination %d, %d, %d, %d\n", pin1, pin2, pin3, pin4);
    test28BYJ48PinCombination(pin1, pin2, pin3, pin4, testSteps, testDelay);
    
    server.send(200, "text/plain", "Pin test completed: " + String(pin1) + ", " + 
                String(pin2) + ", " + String(pin3) + ", " + String(pin4));
    
  } else if (action == "autoTest" && server.hasArg("pins")) {
    String pinListStr = server.arg("pins");
    int testSteps = server.hasArg("testSteps") ? server.arg("testSteps").toInt() : 50;
    int testDelay = server.hasArg("testDelay") ? server.arg("testDelay").toInt() : 5;
    int betweenDelay = server.hasArg("betweenDelay") ? server.arg("betweenDelay").toInt() : 1000;
    int timeout = server.hasArg("timeout") ? server.arg("timeout").toInt() : 10;
    
    // Parse Pin-Liste
    int pins[10];
    int pinCount = 0;
    int startIdx = 0;
    
    for (int i = 0; i <= pinListStr.length(); i++) {
      if (i == pinListStr.length() || pinListStr.charAt(i) == ',') {
        if (i > startIdx && pinCount < 10) {
          pins[pinCount++] = pinListStr.substring(startIdx, i).toInt();
        }
        startIdx = i + 1;
      }
    }
    
    if (pinCount < 4) {
      server.send(400, "text/plain", "Mindestens 4 Pins erforderlich");
      return;
    }
    
    Serial.printf("28BYJ-48: Starting auto-test with %d pins\n", pinCount);
    
    // Starte Auto-Test in separatem Thread (simuliert durch Flag)
    extern bool auto_test_running;
    extern int auto_test_pins[10];
    extern int auto_test_pin_count;
    extern int auto_test_steps;
    extern int auto_test_delay;
    extern int auto_test_between_delay;
    extern int auto_test_timeout;
    
    auto_test_running = true;
    auto_test_pin_count = pinCount;
    auto_test_steps = testSteps;
    auto_test_delay = testDelay;
    auto_test_between_delay = betweenDelay;
    auto_test_timeout = timeout;
    
    for (int i = 0; i < pinCount; i++) {
      auto_test_pins[i] = pins[i];
    }
    
    server.send(200, "text/plain", "Auto-Test gestartet mit " + String(pinCount) + " Pins");
    
  } else if (action == "checkAutoTest") {
    extern bool waiting_for_pin_test_feedback;
    extern bool auto_test_running;
    extern int test_pin_combination[4];
    extern bool pin_test_confirmed;
    
    String jsonResponse = "{";
    
    if (waiting_for_pin_test_feedback) {
      jsonResponse += "\"waitingForFeedback\":true,";
      jsonResponse += "\"pin1\":" + String(test_pin_combination[0]) + ",";
      jsonResponse += "\"pin2\":" + String(test_pin_combination[1]) + ",";
      jsonResponse += "\"pin3\":" + String(test_pin_combination[2]) + ",";
      jsonResponse += "\"pin4\":" + String(test_pin_combination[3]) + ",";
      jsonResponse += "\"testComplete\":false";
    } else if (!auto_test_running) {
      jsonResponse += "\"waitingForFeedback\":false,";
      jsonResponse += "\"testComplete\":true,";
      jsonResponse += "\"success\":" + String(pin_test_confirmed ? "true" : "false");
      
      if (pin_test_confirmed) {
        int p1, p2, p3, p4;
        get28BYJ48PinConfiguration(&p1, &p2, &p3, &p4);
        jsonResponse += ",\"pin1\":" + String(p1);
        jsonResponse += ",\"pin2\":" + String(p2);
        jsonResponse += ",\"pin3\":" + String(p3);
        jsonResponse += ",\"pin4\":" + String(p4);
      }
    } else {
      jsonResponse += "\"waitingForFeedback\":false,";
      jsonResponse += "\"testComplete\":false";
    }
    
    jsonResponse += "}";
    server.send(200, "application/json", jsonResponse);
    
  } else if (action == "confirmAutoTest") {
    extern bool waiting_for_pin_test_feedback;
    extern bool pin_test_confirmed;
    
    waiting_for_pin_test_feedback = false;
    pin_test_confirmed = true;
    
    Serial.println("User confirmed combination");
    server.send(200, "text/plain", "Confirmed");
    
  } else if (action == "rejectAutoTest") {
    extern bool waiting_for_pin_test_feedback;
    extern bool pin_test_confirmed;
    
    waiting_for_pin_test_feedback = false;
    pin_test_confirmed = false;
    
    Serial.println("✗ Benutzer hat Kombination abgelehnt");
    server.send(200, "text/plain", "Rejected");
    
  } else if (action == "moveToPosition" && server.hasArg("position")) {
    int position = server.arg("position").toInt();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 50;
    
    Serial.printf("28BYJ-48: Moving to position %d at speed %d%%\n", position, speed);
    set28BYJ48MotorSpeed(speed);
    move28BYJ48MotorToPosition(position);
    
    server.send(200, "text/plain", "Moving to position: " + String(position));
    
  } else if (action == "moveRelative" && server.hasArg("steps")) {
    int steps = server.arg("steps").toInt();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 50;
    int direction = server.hasArg("direction") ? server.arg("direction").toInt() : 1;
    
    Serial.printf("28BYJ-48: Moving %d steps (direction=%d) at speed %d%%\n", steps, direction, speed);
    set28BYJ48MotorSpeed(speed);
    move28BYJ48MotorWithSpeed(steps, direction, speed);
    
    server.send(200, "text/plain", "Moving " + String(steps * direction) + " steps");
    
  } else if (action == "moveDegrees" && server.hasArg("degrees")) {
    float degrees = server.arg("degrees").toFloat();
    int speed = server.hasArg("speed") ? server.arg("speed").toInt() : 50;
    int direction = server.hasArg("direction") ? server.arg("direction").toInt() : 1;
    
    Serial.printf("28BYJ-48: Rotating %.2f degrees (direction=%d) at speed %d%%\n", degrees, direction, speed);
    set28BYJ48MotorSpeed(speed);
    move28BYJ48MotorDegrees(degrees, direction);
    
    server.send(200, "text/plain", "Rotating " + String(degrees) + " degrees " + (direction == 1 ? "clockwise" : "counter-clockwise"));
    
  } else if (action == "home") {
    Serial.println("28BYJ-48: Homing motor");
    home28BYJ48Motor();
    server.send(200, "text/plain", "Motor homed");
    
  } else if (action == "calibrate") {
    Serial.println("28BYJ-48: Calibrating motor");
    calibrate28BYJ48Motor();
    server.send(200, "text/plain", "Motor calibrated");
    
  } else {
    server.send(400, "text/plain", "Invalid action or missing parameters");
  }
}

// 28BYJ-48 Motor Status Handler
void handle28BYJ48MotorStatus() {
  Motor28BYJ48Status status = get28BYJ48MotorStatus();
  
  // Get current pin configuration
  int pin1, pin2, pin3, pin4;
  get28BYJ48PinConfiguration(&pin1, &pin2, &pin3, &pin4);
  
  String jsonResponse = "{"
    "\"currentPosition\":" + String(status.currentPosition) + ","
    "\"targetPosition\":" + String(status.targetPosition) + ","
    "\"isMoving\":" + String(status.isMoving ? "true" : "false") + ","
    "\"currentSpeed\":" + String(status.currentSpeed) + ","
    "\"isHomed\":" + String(status.isHomed ? "true" : "false") + ","
    "\"pin1\":" + String(pin1) + ","
    "\"pin2\":" + String(pin2) + ","
    "\"pin3\":" + String(pin3) + ","
    "\"pin4\":" + String(pin4) +
    "}";
  
  server.send(200, "application/json", jsonResponse);
}

// Pin Configuration Handler
void handlePinConfig() {
  PinConfiguration config = getPinConfig();
  
  String jsonResponse = "{"
    "\"motor_28byj48\":{\"pin1\":" + String(config.motor_28byj48_pin1) + 
    ",\"pin2\":" + String(config.motor_28byj48_pin2) + 
    ",\"pin3\":" + String(config.motor_28byj48_pin3) + 
    ",\"pin4\":" + String(config.motor_28byj48_pin4) + "},"
    "\"nema23\":{\"step1\":" + String(config.nema23_step_pins[0]) +
    ",\"dir1\":" + String(config.nema23_dir_pins[0]) +
    ",\"enable1\":" + String(config.nema23_enable_pins[0]) +
    ",\"step2\":" + String(config.nema23_step_pins[1]) +
    ",\"dir2\":" + String(config.nema23_dir_pins[1]) +
    ",\"enable2\":" + String(config.nema23_enable_pins[1]) +
    ",\"step3\":" + String(config.nema23_step_pins[2]) +
    ",\"dir3\":" + String(config.nema23_dir_pins[2]) +
    ",\"enable3\":" + String(config.nema23_enable_pins[2]) + "},"
    "\"servo\":{\"pin1\":" + String(config.servo_pins[0]) +
    ",\"pin2\":" + String(config.servo_pins[1]) +
    ",\"pin3\":" + String(config.servo_pins[2]) + "},"
    "\"led\":{\"pin1\":" + String(config.led_pins[0]) +
    ",\"pin2\":" + String(config.led_pins[1]) +
    ",\"pin3\":" + String(config.led_pins[2]) +
    ",\"count1\":" + String(config.led_counts[0]) +
    ",\"count2\":" + String(config.led_counts[1]) +
    ",\"count3\":" + String(config.led_counts[2]) + "},"
    "\"button\":" + String(config.button_pin) + ","
    "\"wifi\":{\"ssid\":\"" + String(config.wifi_ssid) + 
    "\",\"password\":\"" + String(config.wifi_password) + 
    "\",\"hostname\":\"" + String(config.wifi_hostname) + "\"}"
    "}";
  
  server.send(200, "application/json", jsonResponse);
}

// Save Pin Configuration Handler
void handleSavePinConfig() {
  if (server.hasArg("component")) {
    String component = server.arg("component");
    
    if (component == "motor_28byj48") {
      if (server.hasArg("pin1") && server.hasArg("pin2") && server.hasArg("pin3") && server.hasArg("pin4")) {
        int pin1 = server.arg("pin1").toInt();
        int pin2 = server.arg("pin2").toInt();
        int pin3 = server.arg("pin3").toInt();
        int pin4 = server.arg("pin4").toInt();
        
        set28BYJ48Pins(pin1, pin2, pin3, pin4);
        
        // Re-initialize motor with new pins
        setup28BYJ48Motor();
        
        server.send(200, "text/plain", "28BYJ-48 pins saved and applied");
      } else {
        server.send(400, "text/plain", "Missing pin parameters");
      }
    } 
    else if (component == "nema23") {
      if (server.hasArg("stepPin") && server.hasArg("dirPin") && server.hasArg("enablePin")) {
        int motorId = server.hasArg("motorId") ? server.arg("motorId").toInt() : 1;
        int stepPin = server.arg("stepPin").toInt();
        int dirPin = server.arg("dirPin").toInt();
        int enablePin = server.arg("enablePin").toInt();
        
        setNEMA23PinsById(motorId, stepPin, dirPin, enablePin);
        if (!configureAdvancedMotorPinsById(motorId, stepPin, dirPin, enablePin)) {
          server.send(400, "text/plain", "Invalid motorId (1..3)");
          return;
        }
        server.send(200, "text/plain", "NEMA 23 motor " + String(motorId) + " pins saved and applied");
      } else {
        server.send(400, "text/plain", "Missing pin parameters");
      }
    }
    else if (component == "servo") {
      if (server.hasArg("pin")) {
        int servoId = server.hasArg("servoId") ? server.arg("servoId").toInt() : 1;
        int pin = server.arg("pin").toInt();

        if (!isValidOutputPinForServo(pin)) {
          server.send(400, "text/plain", "Invalid servo pin (not an output-capable GPIO)");
          return;
        }

        if (!reconfigureServoPin(servoId, pin)) {
          server.send(400, "text/plain", "Invalid servoId (1..3)");
          return;
        }

        setServoPinById(servoId, pin);
        
        server.send(200, "text/plain", "Servo " + String(servoId) + " pin saved and applied");
      } else {
        server.send(400, "text/plain", "Missing pin parameter");
      }
    }
    else if (component == "led") {
      if (server.hasArg("pin")) {
        int ledId = server.hasArg("ledId") ? server.arg("ledId").toInt() : 1;
        int pin = server.arg("pin").toInt();
        int count = server.hasArg("count") ? server.arg("count").toInt() : 1;

        setLedPinById(ledId, pin);
        setLedCountById(ledId, count);
        setupLEDs();

        server.send(200, "text/plain", "LED output " + String(ledId) + " pin/count saved and applied");
      } else {
        server.send(400, "text/plain", "Missing pin parameter");
      }
    }
    else if (component == "button") {
      if (server.hasArg("pin")) {
        int pin = server.arg("pin").toInt();
        setButtonPin(pin);
        server.send(200, "text/plain", "Button pin saved (restart required)");
      } else {
        server.send(400, "text/plain", "Missing pin parameter");
      }
    }
    else {
      server.send(400, "text/plain", "Invalid component");
    }
  } else {
    server.send(400, "text/plain", "Missing component parameter");
  }
}

// Reset Pin Configuration Handler
void handleResetPinConfig() {
  resetPinConfigToDefaults();
  savePinConfig();
  
  // Module neu initialisieren
  setup28BYJ48Motor();
  setupServo();
  
  server.send(200, "text/plain", "Pin configuration reset (reboot recommended for full application)");
}

// System Info Handler
void handleSystemInfo() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  String wifiSsid = wifiConnected ? WiFi.SSID() : "";
  String wifiIp = wifiConnected ? WiFi.localIP().toString() : "";
  String wifiHostname = String(WiFi.getHostname());
  int wifiRssi = wifiConnected ? WiFi.RSSI() : 0;

  String jsonResponse = "{"
    "\"chipModel\":\"" + String(ESP.getChipModel()) + "\","
    "\"chipCores\":" + String(ESP.getChipCores()) + ","
    "\"freeHeap\":" + String(ESP.getFreeHeap()) + ","
    "\"heapSize\":" + String(ESP.getHeapSize()) + ","
    "\"uptime\":" + String(millis() / 1000) + ","
    "\"flashSize\":" + String(ESP.getFlashChipSize()) + ","
    "\"wifiConnected\":" + String(wifiConnected ? "true" : "false") + ","
    "\"wifiSsid\":\"" + wifiSsid + "\","
    "\"wifiHostname\":\"" + wifiHostname + "\","
    "\"wifiIp\":\"" + wifiIp + "\","
    "\"wifiRssi\":" + String(wifiRssi) +
    "}";
  
  server.send(200, "application/json", jsonResponse);
}

// Save WiFi Configuration Handler
void handleSaveWiFiConfig() {
  if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("hostname")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    String hostname = server.arg("hostname");
    
    // Validierung
    if (ssid.length() == 0 || ssid.length() > 63) {
      server.send(400, "text/plain", "SSID invalid (1-63 characters)");
      return;
    }
    
    if (password.length() > 63) {
      server.send(400, "text/plain", "Password too long (max 63 characters)");
      return;
    }
    
    if (hostname.length() == 0 || hostname.length() > 31) {
      server.send(400, "text/plain", "Hostname invalid (1-31 characters)");
      return;
    }
    
    // Save WiFi configuration
    setWiFiConfig(ssid.c_str(), password.c_str(), hostname.c_str());
    
    server.send(200, "text/plain", "WiFi configuration saved. Restarting in 3 seconds...");
    
    // Short delay, then restart ESP
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// Get Pin Configuration as JSON
void handleGetPinConfig() {
  PinConfiguration config = getPinConfig();
  
  String json = "{";
  json += "\"motor28byj48\":{";
  json += "\"pin1\":" + String(config.motor_28byj48_pin1) + ",";
  json += "\"pin2\":" + String(config.motor_28byj48_pin2) + ",";
  json += "\"pin3\":" + String(config.motor_28byj48_pin3) + ",";
  json += "\"pin4\":" + String(config.motor_28byj48_pin4);
  json += "},";
  json += "\"servo\":{";
  json += "\"pin1\":" + String(config.servo_pins[0]) + ",";
  json += "\"pin2\":" + String(config.servo_pins[1]) + ",";
  json += "\"pin3\":" + String(config.servo_pins[2]);
  json += "},";
  json += "\"led\":{";
  json += "\"pin1\":" + String(config.led_pins[0]) + ",";
  json += "\"pin2\":" + String(config.led_pins[1]) + ",";
  json += "\"pin3\":" + String(config.led_pins[2]) + ",";
  json += "\"count1\":" + String(config.led_counts[0]) + ",";
  json += "\"count2\":" + String(config.led_counts[1]) + ",";
  json += "\"count3\":" + String(config.led_counts[2]);
  json += "},";
  json += "\"button\":" + String(config.button_pin);
  json += "}";
  
  server.send(200, "application/json", json);
}

// Set Servo Pin
void handleSetServoPin() {
  if (server.hasArg("pin")) {
    int servoId = server.hasArg("servoId") ? server.arg("servoId").toInt() : 1;
    int pin = server.arg("pin").toInt();

    if (!isValidOutputPinForServo(pin)) {
      server.send(400, "text/plain", "Invalid servo pin (not an output-capable GPIO)");
      return;
    }

    Serial.printf("Setting servo %d pin to GPIO %d\n", servoId, pin);
    if (!reconfigureServoPin(servoId, pin)) {
      server.send(400, "text/plain", "Invalid servoId (1..3)");
      return;
    }
    setServoPinById(servoId, pin);
    
    server.send(200, "text/plain", "Servo " + String(servoId) + " pin set to GPIO " + String(pin) + " and saved to EEPROM");
  } else {
    server.send(400, "text/plain", "Missing 'pin' parameter");
  }
}

// Set LED Pin
void handleSetLedPin() {
  if (server.hasArg("pin")) {
    int ledId = server.hasArg("ledId") ? server.arg("ledId").toInt() : 1;
    int pin = server.arg("pin").toInt();
    int count = server.hasArg("count") ? server.arg("count").toInt() : getLedCountById(ledId);
    
    Serial.printf("Setting LED output %d pin to GPIO %d (count=%d)\n", ledId, pin, count);
    setLedPinById(ledId, pin);
    setLedCountById(ledId, count);
    setupLEDs();  // Re-initialize hardware
    
    server.send(200, "text/plain", "LED output " + String(ledId) + " pin set to GPIO " + String(pin) + " (count=" + String(count) + ") and saved to EEPROM");
  } else {
    server.send(400, "text/plain", "Missing 'pin' parameter");
  }
}

// Set Button Pin
void handleSetButtonPin() {
  if (server.hasArg("pin")) {
    int pin = server.arg("pin").toInt();
    
    Serial.printf("Setting button pin to GPIO %d\n", pin);
    setButtonPin(pin);
    setupButton();  // Re-initialize hardware
    
    server.send(200, "text/plain", "Button pin set to GPIO " + String(pin) + " and saved to EEPROM");
  } else {
    server.send(400, "text/plain", "Missing 'pin' parameter");
  }
}

// Handle not found (404)
void handleNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

// Get Device Information
void handleGetDeviceInfo() {
  DeviceInfo info = getDeviceInfo();
  
  String json = "{";
  json += "\"deviceName\":\"" + info.deviceName + "\",";
  json += "\"deviceNumber\":\"" + info.deviceNumber + "\",";
  json += "\"configuration\":\"" + info.configuration + "\",";
  json += "\"description\":\"" + info.description + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}

// Set Device Information
void handleSetDeviceInfo() {
  bool updated = false;
  String message = "Device Information aktualisiert: ";
  
  if (server.hasArg("deviceName")) {
    String deviceName = server.arg("deviceName");
    setDeviceName(deviceName.c_str());
    message += "DeviceName, ";
    updated = true;
  }
  
  if (server.hasArg("deviceNumber")) {
    String deviceNumber = server.arg("deviceNumber");
    setDeviceNumber(deviceNumber.c_str());
    message += "DeviceNumber, ";
    updated = true;
  }
  
  if (server.hasArg("configuration")) {
    String configuration = server.arg("configuration");
    setConfiguration(configuration.c_str());
    message += "Configuration, ";
    updated = true;
  }
  
  if (server.hasArg("description")) {
    String description = server.arg("description");
    setDescription(description.c_str());
    message += "Description, ";
    updated = true;
  }
  
  if (updated) {
    // Entferne das letzte Komma und Leerzeichen
    message = message.substring(0, message.length() - 2);
    server.send(200, "text/plain", message);
  } else {
    server.send(400, "text/plain", "Keine Parameter angegeben");
  }
}