#include <SPI.h>
#include <Arduino.h>
#include <Preferences.h>

#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "ButtonGrid.h"

#define MAX_COMMAND_LENGTH 200

enum mode {
    MODE_NORMAL,
    MODE_TESTING,
    MODE_BRIGHTNESS
};

int pinCS = D9; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )

Max72xxPanel matrix = Max72xxPanel(pinCS);
ButtonGrid buttonGrid = ButtonGrid();

unsigned char buttonStates[8];

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

int pressesRemaining[8][8];
const int TEST_PRESS_COUNT = 2; // Default number of presses for each button in testing mode

mode currentMode = MODE_NORMAL;
Preferences preferences;

int brightness = 1; // Default brightness

void ButtonCallback(int x, int y, bool state);

void bootUpLights()
{
    matrix.fillScreen(LOW);
    matrix.write();

    for(int i = 0; i < 8; i++)
    {
        matrix.drawLine(i, 0, i, 7, HIGH);
        matrix.write();
        delay(50);
    }
    
    long endTime = millis() + 200;
    while(millis() < endTime)
    {
        buttonGrid.Update();
    }

    for(int i = 0; i < 8; i++)
    {
        matrix.drawLine(i, 0, i, 7, LOW);
        matrix.write();
        delay(50);
    }
}


void SetPixel(int x, int y, bool state)
{
    bitWrite(buttonStates[y], x, (int)state);

    matrix.drawPixel(x,y,state ? HIGH : LOW);
    matrix.write();    
}

void SetRow(int y, unsigned char state)
{
    buttonStates[y] = state;

    for(int x = 0; x < 8; x++)
    {
        matrix.drawPixel(x, y, bitRead(state, x)? HIGH : LOW);
    }
    matrix.write();
}

void SetCol(int x, unsigned char state)
{
    for(int y = 0; y < 8; y++)
    {
        int bit = bitRead(state, y);
        bitWrite(buttonStates[y], x, bit) ;        
        matrix.drawPixel(x, y, bit ? HIGH : LOW);
    }
    matrix.write();
}

void BrightnessModeButtonCallback(int x, int y, bool state)
{
    if(state)
    {
        if(y == 6)
        {
            brightness = (8 + x);
            Serial.print("/grid/brightness ");
            Serial.println(brightness); 
            matrix.setIntensity(brightness);            
        }
        else if(y == 7)
        {
            brightness = x;
            Serial.print("/grid/brightness ");
            Serial.println(brightness); 
            matrix.setIntensity(brightness);
        }
        else if(x == 0 && y == 0)
        {
            preferences.putInt("brightness", brightness);
            preferences.end();

            matrix.fillScreen(HIGH);
            matrix.write();
            delay(200);

            matrix.fillScreen(LOW);
            matrix.write();        
            
            currentMode = MODE_NORMAL;
            buttonGrid.RegisterButtonCallback(ButtonCallback);
        }
    }
}

void TestingModeButtonCallback(int x, int y, bool state)
{
    if(state)
    {
        pressesRemaining[x][y]--;
        if(pressesRemaining[x][y] < 0)
        {
            pressesRemaining[x][y] = 0;
        }

        Serial.print("/test/key ");
        Serial.print(x);
        Serial.print(" ");
        Serial.print(y);
        Serial.print(" ");
        Serial.println(pressesRemaining[x][y]);
    }
    else
    {
        bool allZero = true;
        for(int i = 0; i < 8; i++)
        {
            for(int j = 0; j < 8; j++)
            {
                if(pressesRemaining[i][j] != 0)
                {
                    allZero = false;
                    break;
                }
            }
        }

        if(allZero)
        {
            Serial.println("/test/complete");

            matrix.fillScreen(HIGH);
            matrix.write();
            delay(200);

            matrix.fillScreen(LOW);
            matrix.write();        
            
            currentMode = MODE_NORMAL;
            buttonGrid.RegisterButtonCallback(ButtonCallback);
        }        
    }
}


void TestingModeUpdateLeds()
{
    for(int y = 0; y < 8; y++)
    {
        for(int x = 0; x < 8; x++)
        {
            if(pressesRemaining[x][y] >= TEST_PRESS_COUNT)
            {
                matrix.drawPixel(x, y, HIGH);
            }
            else if(pressesRemaining[x][y] == 0)
            {
                matrix.drawPixel(x, y, LOW);
            }
            else 
            {
                uint16_t color = (millis() / 100) % (1 + pressesRemaining[x][y]) == 0 ? HIGH : LOW;
                matrix.drawPixel(x, y, color);
            }
        }
    }
    matrix.write();
}

void BrightnessModeUpdateLeds()
{
    matrix.drawLine(0, 6, 7, 6, HIGH); // Brightness row
    matrix.drawLine(0, 7, 7, 7, HIGH); // Brightness row

    int x = (brightness) % 8;
    int y = 7 - (brightness / 8);
    matrix.drawPixel(x, y, millis() / 200 % 2 == 0 ? HIGH : LOW); // Brightness mode indicator    

    matrix.drawPixel(0, 0, millis() / 400 % 2 == 0 ? HIGH : LOW); // Brightness mode indicator

    matrix.write();
}
void ButtonModeCallback(int x, int y, bool state)
{
    if(state)
    {

        if(x == 0 && y == 0)
        {
            // Toggle mode
            currentMode = MODE_BRIGHTNESS;
        }
        else if(x == 1 && y == 0)
        {
            currentMode = MODE_TESTING;
        }
    }
    else
    {
        if(x == 0 && y == 0)
        {
            // Toggle mode
            currentMode = MODE_NORMAL;
        }
        else if(x == 1 && y == 0)
        {
            currentMode = MODE_NORMAL;
        }        
    }


}

void ButtonCallback(int x, int y, bool state)
{
    bitWrite(buttonStates[y], x, (int)state);

    Serial.print("/grid/key ");
    Serial.print(x);
    Serial.print(" ");
    Serial.print(y);
    Serial.print(" ");
    Serial.println(state);
}


void ProcessSerial()
{
    const char delimiter[2] = " ";

    // Use a non-const char* for strtok
    char inputBuffer[MAX_COMMAND_LENGTH];
    strncpy(inputBuffer, inputString.c_str(), sizeof(inputBuffer));
    inputBuffer[sizeof(inputBuffer) - 1] = '\0'; // Ensure null-termination

    char* command = strtok(inputBuffer, delimiter);

    if(strcmp(command, "/grid/led/set") == 0)
    {
        char* xToken = strtok(NULL, delimiter);
        char* yToken = strtok(NULL, delimiter);
        char* sToken = strtok(NULL, delimiter);

        if(xToken != NULL && yToken != NULL && sToken != NULL)
        {   
            int x = atoi(xToken);
            int y = atoi(yToken);
            bool s = atoi(sToken) == 1;

            SetPixel(x, y, s);
        }
    }
    else if(strcmp(command, "/grid/led/row") == 0)
    {
        char* yToken = strtok(NULL, delimiter); 
        char* sToken = strtok(NULL, delimiter);

        if(yToken != NULL && sToken != NULL)
        {
            int y = atoi(yToken);
            unsigned int s = atoi(sToken);

            SetRow(y, s);
        }
    }
    else if(strcmp(command, "/grid/led/col") == 0)
    {
        char* xToken = strtok(NULL, delimiter);
        char* sToken = strtok(NULL, delimiter);

        if(xToken != NULL && sToken != NULL)
        {
            int x = atoi(xToken);
            unsigned int s = atoi(sToken);

            SetCol(x, s);
        }
    }    
    else if(strcmp(command, "/grid/led/all") == 0)
    {
        char* sToken = strtok(NULL, delimiter);

        if(sToken != NULL)
        {
            bool s = atoi(sToken) == 1;

            Serial.println(s);
            for(int y = 0; y < 8; y++)
            {
                SetRow(y, s == 1 ? 255 : 0);
            }
        }
    }   
    else if(strcmp(command, "/grid/led/map") == 0)
    {
        char* xToken = strtok(NULL, delimiter);
        char* yToken = strtok(NULL, delimiter); 
        char* sTokens[8];

        bool isValid = (xToken != NULL && yToken != NULL);

        for(int i = 0; i < 8; i++)
        {
            sTokens[i] = strtok(NULL, delimiter);
            isValid &= sTokens[i] != NULL;
        }

        if(isValid)
        {
            for(int y = 0; y < 8; y++)
            {
                SetRow(y, atoi(sTokens[y]));
            }
        }
    }
    else
    {
        Serial.print("/grid/error ");
        Serial.println(inputString);
    }      

    stringComplete = false;
    inputString = "";
}

void UpdateSerial() 
{
    while (Serial.available()) 
    {
        char inChar = (char)Serial.read();
        inputString += inChar;
        if (inChar == '\n') 
        {
            stringComplete = true;
        }
    }
}



void setup() 
{
    Serial.begin(115200);

    // get the brightness from 
    preferences.begin("buttonGrid", false); // namespace, read-only flag
    brightness = preferences.getInt("brightness", brightness); // key, default value

    matrix.setIntensity(brightness); // Set brightness between 0 and 15
    matrix.setRotation(1);
    
    #ifdef FLIP_LED
    matrix.setFlip(true, false);
    #endif
    
    matrix.fillScreen(LOW);
    matrix.write();
    
    buttonGrid.Setup();
    
    currentMode = MODE_NORMAL;
    buttonGrid.RegisterButtonCallback(ButtonModeCallback);
    
    bootUpLights();

    Serial.print("/grid/brightness ");
    Serial.println(brightness);
    
    for(int i = 0; i < 8; i++)
    {
        buttonStates[i] = 0;
    }
    
    // reserve bytes for the inputString:
    inputString.reserve(MAX_COMMAND_LENGTH);  
    
    if(currentMode == MODE_TESTING)
    {
        Serial.println("/grid/mode testing");
        for(int x = 0; x < 8; x++)
        {
            for(int y = 0; y < 8; y++)
            {
                pressesRemaining[x][y] = TEST_PRESS_COUNT;
            }
        }

        buttonGrid.RegisterButtonCallback(TestingModeButtonCallback);

    }
    else if(currentMode == MODE_BRIGHTNESS)
    {
        Serial.println("/grid/mode brightness");
        buttonGrid.RegisterButtonCallback(BrightnessModeButtonCallback);
    }
    else
    {
        buttonGrid.RegisterButtonCallback(ButtonCallback);
    }
}

void loop() 
{
    buttonGrid.Update();

    if(currentMode == MODE_TESTING)
    {
        TestingModeUpdateLeds();
    }
    else if(currentMode == MODE_BRIGHTNESS)
    {
        BrightnessModeUpdateLeds();
    }
    else
    {
        UpdateSerial();
    }
}
