#ifndef BUTTON_GRID_H
#define BUTTON_GRID_H

#include <Arduino.h>

#define GRID_WIDTH 8
#define GRID_HEIGHT 8



// SN74HC165N -> PISO
#define DEFAULT_PISO_DATA D3
#define DEFAULT_PISO_LOAD D4
#define DEFAULT_PISO_CLOCK D5

// SN74HC164N -> SIPO
#define DEFAULT_SIPO_DATA D0
#define DEFAULT_SIPO_CLEAR D1
#define DEFAULT_SIPO_CLOCK D2

// ----- Callback function types -----
extern "C" {
typedef void (*callbackFunction)(int, int, bool);
}

struct ButtonStateInfo
{
    unsigned int pressTime = 0;
    bool state = false;
    bool debounceComplete = false;
};


class ButtonGrid
{
public:
    
    ButtonGrid();

    void Setup();
    void RegisterButtonCallback(callbackFunction onButtonStateChange);
    
    void Update();

private:
    
    ButtonStateInfo _buttons[GRID_WIDTH][GRID_HEIGHT];

    int _pisoClockPin = DEFAULT_PISO_CLOCK;
    int _pisoDataPin = DEFAULT_PISO_DATA;
    int _pisoLoadPin = DEFAULT_PISO_LOAD;

    int _sipoClockPin = DEFAULT_SIPO_CLOCK;
    int _sipoDataPin = DEFAULT_SIPO_DATA;
    int _sipoClearPin = DEFAULT_SIPO_CLEAR;

    unsigned int _currentOffset = 0;

    callbackFunction _buttonStateCallback = 0;

    void UpdateButtonStates();

    unsigned char ReadByte();

    void ResetOffset();
    void IncrementOffset();

    void UpdateButtonState(int x, int y, bool state);
  
};

#endif //BUTTON_GRID_H
