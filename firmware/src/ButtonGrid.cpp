#include "ButtonGrid.h"
#include <Arduino.h>

#define DELAY_MICROSECONDS 5
#define DEBOUNCE_TIME 20

ButtonGrid::ButtonGrid()
{}

void ButtonGrid::Setup()
{
    pinMode(_pisoLoadPin, OUTPUT); //Set out IC and button pins
    pinMode(_pisoDataPin, INPUT);
    pinMode(_pisoClockPin, OUTPUT);

    pinMode(_sipoClearPin, OUTPUT);
    pinMode(_sipoClockPin, OUTPUT);
    pinMode(_sipoDataPin, OUTPUT);

    digitalWrite(_pisoLoadPin, LOW);
    digitalWrite(_pisoDataPin, LOW);    
    digitalWrite(_pisoClockPin, LOW);    

    digitalWrite(_sipoClearPin, HIGH);
    digitalWrite(_sipoClockPin, LOW);    
    digitalWrite(_sipoDataPin, LOW);  
    
    // Reset all button states
    for(int x = 0; x < GRID_WIDTH; x++)
    {
        for(int y = 0; y < GRID_HEIGHT; y++)
        {
            _buttons[x][y].state = false;
            _buttons[x][y].debounceComplete = false;
            _buttons[x][y].pressTime = 0;
        }
    }

    ResetOffset();
}

void ButtonGrid::Update()
{
    UpdateButtonStates();
}

void ButtonGrid::UpdateButtonStates()
{
    for(int x = 0; x < 8; x++)
    {     
        
        unsigned char newButtonStates = ReadByte();
        
        for(int y = 0; y < 8; y++)
        {
            UpdateButtonState(x, (7-y), (newButtonStates >> y & 1) == LOW );
        }
        
        
        IncrementOffset();   
    }

}

unsigned char ButtonGrid::ReadByte()
{
    byte data = 0;

    digitalWrite(_pisoClockPin, HIGH);  //First step here will be to pulse the latch/reset pin while sending a falling clock signal.
    digitalWrite(_pisoLoadPin, LOW);    //This will lock in the current state of the inputs to be sent to the arduino. 

    delayMicroseconds(DELAY_MICROSECONDS);

    digitalWrite(_pisoClockPin, LOW);
    digitalWrite(_pisoLoadPin, HIGH); 

    for(int i = 0; i < 8; i++)
    {
        bitWrite(data, i, digitalRead(_pisoDataPin)); //Grab our byte, select which bit, and write the current input as 1 or 0

        digitalWrite(_pisoClockPin, LOW); 
        delayMicroseconds(DELAY_MICROSECONDS);
        digitalWrite(_pisoClockPin, HIGH); //send a clock leading edge so to load the next bit
        delayMicroseconds(DELAY_MICROSECONDS);
    } 
    
    return data;
}

void ButtonGrid::ResetOffset()
{
    digitalWrite(_sipoClockPin, LOW);
    
    digitalWrite(_sipoClearPin, LOW);
    delayMicroseconds(DELAY_MICROSECONDS);
    digitalWrite(_sipoClearPin, HIGH);

    unsigned int values[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, LOW};

    for(int i = 0; i < 8; i++)
    {
        digitalWrite(_sipoClockPin, LOW); 
        digitalWrite(_sipoDataPin, values[i]);
        delayMicroseconds(DELAY_MICROSECONDS);

        digitalWrite(_sipoClockPin, HIGH); //send a clock leading edge so to load the next bit
        delayMicroseconds(DELAY_MICROSECONDS);
    }

    _currentOffset = 0;    
}

void ButtonGrid::IncrementOffset()
{
    _currentOffset = (_currentOffset + 1) % 8;

    digitalWrite(_sipoClockPin, LOW);
    digitalWrite(_sipoDataPin, _currentOffset == 0 ? LOW : HIGH);

    delayMicroseconds(DELAY_MICROSECONDS);
    
    digitalWrite(_sipoClockPin, HIGH); //send a clock leading edge so to load the next bit

    delayMicroseconds(DELAY_MICROSECONDS);
}

void ButtonGrid::RegisterButtonCallback(callbackFunction onButtonStateChange)
{
    _buttonStateCallback = onButtonStateChange;
}


void ButtonGrid::UpdateButtonState(int x, int y, bool state)
{
    if(state != _buttons[x][y].state)
    {
        if(state == true)
        {
            _buttons[x][y].pressTime = millis();
        }
        else
        {
            if(_buttons[x][y].debounceComplete)
            {
                if(_buttonStateCallback != NULL)
                {
                    _buttonStateCallback(x,y,false);
                }
            }

            _buttons[x][y].debounceComplete = false;
        }

        _buttons[x][y].state = state;
    }
    else
    {
        if(_buttons[x][y].state == true && _buttons[x][y].debounceComplete == false && millis() - _buttons[x][y].pressTime > DEBOUNCE_TIME)
        {         
            if(_buttonStateCallback != NULL)
            {
                _buttonStateCallback(x,y,true);
            }

            _buttons[x][y].debounceComplete = true;
        }
    }
}