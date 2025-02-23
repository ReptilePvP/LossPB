# Loss Prevention Log - M5Stack CoreS3 Project

## Required Libraries
- M5CoreS3.h
- lvgl.h (v8.4.0)
- ArduinoIoTCloud.h
- Arduino_ConnectionHandler.h
- WiFi.h

## Hardware Requirements
- M5Stack CoreS3
- M5Stack Dual Button & Key Unit (Connected to PIN 8)

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
   - `colorMenu`: Shared menu for all color selections
   - `itemMenu`: Stolen item selection
   - `confirmScreen`: Entry confirmation
   - `wifiSettingsScreen`: WiFi settings and network selection

2. **Custom Styles**
   - Material design inspired buttons
   - Scrollable containers with custom scrollbars
   - WiFi status indicator (green when connected, red when disconnected)
   - Debug output in Serial Monitor when DEBUG_ENABLED is true

### Color Options
```cpp
// Shirt Colors (18 options)
const char* shirtColors[] = {
    "White", "Black", "Blue", "Red", "Green", "Yellow", 
    "Pink", "Purple", "Orange", "Grey", "Brown", "Navy",
    "Maroon", "Teal", "Beige", "Burgundy", "Olive", "Lavender"
};

// Pants Colors (16 options)
const char* pantsColors[] = {
    "Black", "Blue", "Grey", "Khaki", "Brown", "Navy",
    "White", "Beige", "Olive", "Burgundy", "Tan", "Charcoal",
    "Dark Blue", "Light Grey", "Stone", "Cream"
};

// Shoe Colors (16 options)
const char* shoeColors[] = {
    "Black", "Brown", "White", "Grey", "Navy", "Tan",
    "Red", "Blue", "Multi-Color", "Gold", "Silver", "Bronze",
    "Beige", "Burgundy", "Pink", "Green"
};
```

### Key Features

#### Scrolling with Key Unit
- Uses M5Stack Key Unit on PIN 8 for scrolling
- Scroll amount: 40 pixels per button press
- Automatic scroll limits detection
- Smooth scrolling animation

#### WiFi Indicator
- Top-right corner placement
- Green WiFi symbol when connected
- Red X symbol when disconnected
- Real-time status updates

#### Debug System
```cpp
#define DEBUG_ENABLED true
#define DEBUG_PRINT(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); }
#define DEBUG_PRINTF(x, ...) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.printf(x, __VA_ARGS__); }
```

### Display Configuration
- Screen dimensions: 320x240 pixels
- Color depth: 16-bit with swap (LV_COLOR_16_SWAP = 1)
- Display buffer: SCREEN_WIDTH * 20
- Landscape orientation (rotation = 1)
- Brightness: 100

### Cloud Configuration
```cpp
// Arduino Cloud credentials
const char DEVICE_LOGIN_NAME[] = "45444c7a-9c6a-42ed-8c94-413e098a2b3d";
const char DEVICE_KEY[] = "mIB9Ezs8tfWtqe8Mx2qhQgYxT";
```

## Usage Notes
1. Debug output can be enabled/disabled via DEBUG_ENABLED flag
2. Key Unit scrolling works on all scrollable menus
3. WiFi indicator shows real-time connection status
4. Color menus support more options than visible, requiring scrolling
5. Menu transitions use async calls to prevent crashes
6. All UI elements use the Montserrat font family

## Development Notes
- Touch coordinates are calibrated for the CoreS3 screen
- Scrolling uses LVGL's built-in animation system
- WiFi connection status is monitored continuously
- Debug messages include timestamps and context
- Memory management optimized for menu transitions

## Common Debug Messages
```
Creating Shirt Color Menu
Shirt Color Menu grid created and set as scroll object
Number of shirt colors: 18
Selected shirt color: Blue
Current entry: Blue,
Creating pants menu after shirt selection
```

## Known Issues & Solutions
1. Menu Transition Crashes:
   - Fixed by using lv_async_call for transitions
   - Proper cleanup of scroll objects
   - Debug tracking of menu state

2. Scroll Limitations:
   - Implemented Key Unit support
   - Added scroll bounds checking
   - Smooth animation for better UX

## Future Improvements
1. Add scroll up functionality with second Key Unit button
2. Implement scroll acceleration for longer lists
3. Add haptic feedback for button presses
4. Enhance debug logging with more context
5. Add error recovery mechanisms
