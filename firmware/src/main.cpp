#include <SPI.h>
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Max72xxPanel.h>
#include "ButtonGrid.h"

#define MAX_COMMAND_LENGTH 200

int pinCS = D9; // Attach CS to this pin, DIN to MOSI and CLK to SCK (cf http://arduino.cc/en/Reference/SPI )

Max72xxPanel matrix = Max72xxPanel(pinCS);
ButtonGrid buttonGrid = ButtonGrid();

unsigned char buttonStates[8];

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

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
    
    delay(200);

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
    delay(1000); // Allow time for serial monitor to connect

    matrix.setIntensity(15); // Set brightness between 0 and 15
    matrix.setRotation(1);
    matrix.fillScreen(LOW);
    matrix.write();

    buttonGrid.Setup();
    buttonGrid.RegisterButtonCallback(ButtonCallback);
    
    for(int i = 0; i < 8; i++)
    {
        buttonStates[i] = 0;
    }
    
    // reserve bytes for the inputString:
    inputString.reserve(MAX_COMMAND_LENGTH);  
    
    //bootUpLights();
}

void loop() 
{
    buttonGrid.Update();    
}
