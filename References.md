# Loss Prevention Log - M5Stack CoreS3 Project

## Required Libraries
- Wire.h
- M5Unified.h
- M5GFX.h
- lvgl.h
- ArduinoIoTCloud.h
- Arduino_ConnectionHandler.h
- WiFi.h

## Hardware Requirements
- M5Stack CoreS3
- M5Stack Dual Button & Key Unit

## Project Structure

### Main Components
1. **User Interface**
   - Main Menu
   - Gender Selection
   - Color Selection (Shirt, Pants, Shoes)
   - Item Selection
   - Confirmation Screen
   - WiFi Settings Screen

2. **Cloud Integration**
   - Arduino IoT Cloud connectivity
   - Real-time data synchronization
   - Offline data storage capability

### UI Elements
1. **Screens**
   - `mainScreen`: Home screen with main options
   - `genderMenu`: Gender selection menu
   - `shirtMenu`: Shirt color selection
   - `pantsMenu`: Pants color selection
   - `shoeMenu`: Shoe color selection
   - `itemMenu`: Stolen item selection
   - `confirmScreen`: Entry confirmation
   - `wifiScreen`: WiFi settings and network selection

2. **Custom Styles**
   - Button style with consistent appearance
   - Scrollable containers with custom scrollbars
   - Color-coded WiFi signal strength indicators

### Key Functions

#### Screen Creation
- `createMainMenu()`: Creates the main menu interface
- `createGenderMenu()`: Displays gender selection options
- `createColorMenu()`: Generic color selection menu creator
- `createItemMenu()`: Shows available item options
- `createConfirmScreen()`: Shows entry confirmation
- `createWifiScreen()`: WiFi network selection interface

#### WiFi & Cloud
- `connectWifi()`: Handles WiFi connection process
- `disconnectWifi()`: Manages WiFi disconnection
- `updateWifiIndicator()`: Updates WiFi status display
- `wifiButtonCallback()`: Handles WiFi selection events
- `wifiConnectCallback()`: Manages connection attempts

#### Data Management
- `getTimestamp()`: Generates current timestamp
- `getFormattedEntry()`: Formats entry data
- `saveEntry()`: Saves entry to Arduino Cloud
- `initProperties()`: Initializes cloud properties

### Display Configuration
- Screen dimensions: 320x240 pixels
- Custom button style with:
  - Background color: #404040
  - Border color: #FFFFFF
  - Border width: 2px
  - Border radius: 5px
  - Text color: #FFFFFF
  - Font: Unscii 16

### Color Options
Available colors for clothing items:
```cpp
const char* colors[] = {
    "Red", "Blue", "Green",
    "Yellow", "Black", "White",
    "Brown", "Gray", "Purple"
};
```

### Item Options
Available items for loss reporting:
```cpp
const char* items[] = {
    "Wallet", "Phone", "Keys",
    "Bag", "Watch", "Jewelry",
    "Other"
};
```

## Usage Notes
1. The application requires initial WiFi setup for cloud connectivity
2. Entries can be made offline and will sync when connected
3. The UI is optimized for touch interaction
4. All screens feature a consistent navigation pattern
5. The WiFi settings screen includes signal strength indicators
6. The confirmation screen shows a formatted summary of the entry

## Development Notes
- Touch coordinates are adjusted for screen boundaries
- Scrolling containers use custom styling for better visibility
- WiFi connection status is continuously monitored
- Cloud synchronization can be paused during WiFi configuration

// Arduino Cloud credentials (replace with your own)
const char DEVICE_LOGIN_NAME[] = "45444c7a-9c6a-42ed-8c94-413e098a2b3d"; // From Arduino Cloud Device setup
const char SSID[] = "Wack House";                   // Your Wi-Fi SSID
const char PASS[] = "justice69";              // Your Wi-Fi password
const char DEVICE_KEY[] = "mIB9Ezs8tfWtqe8Mx2qhQgYxT";      // From Arduino Cloud Device setup