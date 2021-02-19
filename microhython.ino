/*
    microhython - Python in your hand - Main code for Arduino side
    Date created: 13/2/2021
    Copyright @raspiduino 2020 - 2021
    -------------------------------------------------------------------------------
    Originally from previous version https://github.com/raspiduino/upythoncomputer
    Now we are renewing the project. The purpose is to create a portable device
    that run Micropython and you can bring it everywhere you want as a computer.

    In the future we might create more utilities for this Micropython port, to make
    this a real computer, not just a simple Python interpreter
    -------------------------------------------------------------------------------
    This program require an Arduino UNO, a Nokia 5110 LCD and 5 buttons.
    -------------------------------------------------------------------------------
    microhython.ino - Main ino file for this project
    Date created 13/2/2021
    Edited using Arduino IDE
*/

#include <AltSoftSerial.h> // Software serial library
#include <PCD8544_HOANGSA.h> // Nokia 5110 LCD library

//#define debug 1 // For debug only, if you use normally, comment this line

// Create software serial object for communicating with esp8266
AltSoftSerial esp8266;

/*
    Some notes about AltSoftSerial:
    When use AltSoftSerial library, the software serial RXD and TXD pin is fixed.
    RXD pin is digital pin 8, and TXD is digital pin 9. Please see note in this
    library.
    -----------------------------------------------------------------------------
    You may ask why we have to use this instead of the standard software serial
    library. The reason is, the standard software serial library is incomptible
    with the LCD interface, so this will cause the LCD unable to display. I have
    faced this problem half year ago.
*/

/*
    Some notes about the data send to esp8266:
    Normally, the text from the user's typing is sent to esp8266 directly, and
    micropython will execute that text as code. But some special key will be
    sent by this program to the esp8266's side and our program will use that
    key for handling the terminal, such as scroll up/down. These keys are:

    ':UP'   : Scroll up
    ':DOWN' : SCroll down

    User can also manually send these using virtual keyboard to scroll up/down.
*/

/************************************LCD setting************************************/

// Define lcd pin, you can change to fit your wiring
#define RST 14
#define CS 15
#define DC 16
#define DIN 17
#define CLK 18

// Create an instance of LCD class
PCD8544 lcd(RST,CS,DC,DIN,CLK); //Setup lcd pin: RST, CS, DC, DIN, CLK.

// Set the LCD setting
#define contrast 25 // Set the LCD contrast from 0 -> 127 (0x00 -> 0x7f)
#define negative 0  // Enable(1) negative feature or not(0). Default is disabled(0)
#define rotation 0  // Not availble, please set this to 0
#define mirror 0    // Not availble, please set this to 0
#define bias 4      // Bias (0 -> 10 or 0x00 -> 0x10), default is 4. Please keep this setting

// User interface flags
bool vkeyboard = false;
int page = 1;
int current_key = 32;

String command;
int charloc = 0;
int startloc = 0;
int endloc = 0;

// Display varriables
int termx = 0;
int termy = 0;

int charx = 0;
int chary = 11;

// Functions
void displaykeyboard(int beginnum, int endnum);
void controlkeyboard();
void refreshkeyboard();
void displayterminal(int incomingByte);
void controlpage();
void updatecontrolpage(int current_select);

/************************************Keyboard setting************************************/

/*
    Some notes about this keyboard
    This keyboard have only 5 buttons! You can imagine it like this:
                        Up
                       _____
                      |  ^  |
              _____   |__|__|   _____
             |     |  |     |  |     |
      Left   |_<-__|  |_OK__|  |__->_|    Right
                      |     |
                      |__↓__|
                       Down
    These 5 buttons is used to select key from the virtual keyboard, scroll
    up/down throught the serial,...

    The buttons will be set as INPUT_PULLUP and will be software debounced.
    -------------------------------------------------------------------------
    About the buttons usage:
    On the terminal, without the keyboard, you can use the up/down button to
    scroll up/down. If you press the OK button, it will show the keyboard and
    the input box for you. Then you can use the up/down, left right button to
    select the key, then press ok to confirm that key. When you finish,
    select the OK in the keyboard, not the OK button.
*/

// Pins of button. Yout can edit this to fit your wiring.
#define BUTTON_LEFT 3
#define BUTTON_OK 4
#define BUTTON_RIGHT 5
#define BUTTON_UP 6
#define BUTTON_DOWN 7

#define debouncetime 200 // Debounce time for button

void setup() {
    // Begin serial communication with esp8266
    esp8266.begin(9600); // Baud 9600

    // Reset esp8266 (esp8266's RST pin is connected to Arduino's digital pin 2)
    pinMode(2, OUTPUT); // Set the pin 2 to be output
    // Reset esp8266
    digitalWrite(2, LOW);
    digitalWrite(2, HIGH);

    // Start the lcd
    lcd.ON(); // Turn on the LCD 
    lcd.SET(contrast, negative, rotation, mirror, bias); // Setup the LCD settings   

    // Display welcome banner
    lcd.Asc_String(0, 0, Asc("Microhython by @raspiduino 2021"), BLACK);
    lcd.Asc_String(0, 32, Asc("Booting up..."), BLACK);
    lcd.Display();
    
    // Pinmode all input keyboard pins as INPUT_PULLUP
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_OK, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);

    // Send enter key to esp8266
    esp8266.println("");

    // Clear the LCD
    delay(1000);
    lcd.Clear();
    lcd.Display();
}

void loop() {
    /************************************ Display the incomming data from esp8266 ************************************/
    while (esp8266.available() > 0) { //If the serial has incomming character, read it
        int incomingByte = esp8266.read(); //Read the character ASCII number to the incomingByte varrible
        if (incomingByte != -1 | incomingByte != 10) {
            // Skip unreadable key
            displayterminal(incomingByte); // Display the char
        }
    }
    
    /******************************************* Check for key press event *******************************************/
    if (!vkeyboard){
        // If not displaying virtual keyboard menu -> in terminal
        if (digitalRead(BUTTON_UP) == LOW){
            // If 'UP' button pressed -> Scroll the terminal up
            esp8266.println(":UP");
            delay(debouncetime);
        }
        
        if (digitalRead(BUTTON_DOWN) == LOW){
            // If 'DOWN' button is pressed -> Scroll the terminal down
            esp8266.println(":DOWN");
            delay(debouncetime);
        }

        if (digitalRead(BUTTON_OK) == LOW){
            // If 'OK' button is pressed -> Show the virtual keyboard
            vkeyboard = true;
            displaykeyboard(32,64); // Call the display keyboard function
            delay(debouncetime);
        }
    } else{
        controlkeyboard(); // Control virtual keyboard events
    }
}

/******************************************** Functions *******************************************/
void displaykeyboard(int beginnum, int endnum){
    int keyx = 1;
    int keyy = 3;
    int cmdx = 0;
    // Display the virtual keyboard
    lcd.Clear(); // Cleat the LCD
    lcd.DrawLine(0,9,85,9,BLACK); // Draw the line

    if(command.length() > 0){
        // Display the input string
        for(int i = startloc; i <= endloc; i++){
            lcd.Asc_Char(cmdx,0,command.charAt(i),BLACK);
            cmdx = cmdx + 5;
        }
    }
    
    //lcd.Asc_String(0, 11, keypage, BLACK); // Display the keyboard

    for(int i = beginnum; i < endnum; i++){
        if (i%8 == 0){
            keyx = 1;
            keyy = keyy + 8;
            lcd.Asc_Char(keyx, keyy, char(i), BLACK);
        }
        else{
            if (i%8 == 7){
                keyx = keyx + 10;
                lcd.Asc_Char(keyx, keyy, char(i), BLACK);
            }
            else {
                keyx = keyx + 10;
                lcd.Asc_Char(keyx, keyy, char(i), BLACK);
            }
        }
        //esp8266.println(keyx);
    }
    
    // Draw text select box
    //lcd.DrawLine(charx, chary, charx, chary + 8, BLACK);
    //lcd.DrawLine(charx, chary, charx + 6, chary, BLACK);
    //lcd.DrawLine(charx + 6, chary, charx + 6, chary + 8, BLACK);
    //lcd.DrawLine(charx, chary + 8, charx + 6, chary + 8, BLACK);
    lcd.Rect(charx-1, chary-1, 9, 11, BLACK);
    
    lcd.Display();
}

void updatecontrolpage(int current_select){
    int cmdx = 0;
    lcd.Clear();
    lcd.DrawLine(0,9,85,9,BLACK); // Draw the line
    
    if(command.length() > 0){
        // Display the input string
        for(int i = startloc; i <= endloc; i++){
            lcd.Asc_Char(cmdx,0,command.charAt(i),BLACK);
            cmdx = cmdx + 5;
        }
    }

    lcd.Asc_String(0,11,Asc("<-"),BLACK);
    lcd.Asc_String(0,19,Asc("->"),BLACK);
    lcd.Asc_String(0,27,Asc("DELETE"),BLACK);
    lcd.Asc_String(0,35,Asc("RESET"),BLACK);
    lcd.Asc_String(0,43,Asc("ENTER"),BLACK);

    lcd.Rect(0, (current_select*8)+3, 35, 8, BLACK);
    
    lcd.Display();
}

void controlpage(){
    int current_select = 1;
    updatecontrolpage(1);
    while(1){
        if(digitalRead(BUTTON_LEFT) == LOW | digitalRead(BUTTON_UP) == LOW){
            if(current_select > 1){
                current_select--;
                updatecontrolpage(current_select);
            }
            else{
                page--;
                refreshkeyboard();
                break;    
            }
        }
        
        if(digitalRead(BUTTON_OK) == LOW){
            if(current_select == 1 && charloc > 0){
                // Change the cusor left
                if(command.length() > 0){
                    charloc--; 
                }
                
            }
            if(current_select == 2 && charloc < command.length()){
                // Change the cusor right
                if(charloc < command.length()){
                    charloc++;  
                }
                   
            }
            if(current_select == 3){
                // Delete char
                if(command.length() > 0){
                    command.remove(charloc-1,1);
                    charloc--;
                    //startloc--;
                    endloc--;
                    updatecontrolpage(3);
                }
                
            }
            if(current_select == 4){
                // Reset the current_key var and turn the keyboard back to page 1 :D useful when the keyboard become lag
                current_key = 32; // Reset it
                page = 1;
                charx = 0;
                chary = 11;
                refreshkeyboard(); // Return to page 1
                current_key = 32;
                break;    
            }
            if(current_select == 5){
                // Send the data to the serial esp8266 and back to terminal
                esp8266.println(command); // Send command to serial
                vkeyboard = false;
                esp8266.println(":DOWN");
                // User interface flags
                page = 1;
                current_key = 32;
                
                command = "";
                charloc = 0;
                startloc = 0;
                endloc = 0;
                
                // Display varriables
                termx = 0;
                termy = 0;
                
                charx = 0;
                chary = 11;
                lcd.Clear();
                lcd.Display();
                break;
            }
        }
        
        if(digitalRead(BUTTON_RIGHT) == LOW | digitalRead(BUTTON_DOWN) == LOW){
            if(current_select < 5){
                current_select++;
                updatecontrolpage(current_select);    
            }
        }
        delay(debouncetime);
    }
}

void refreshkeyboard(){
    switch(page){
        case 1:
            displaykeyboard(32,64);
            break;
        case 2:
            displaykeyboard(64,96);
            break;   
        case 3:
            displaykeyboard(96, 127);
            break;
        case 4:
            // Special page to control the input
            controlpage();
            break;
    }
}

void controlkeyboard(){
    // Control the keyboard events
    if (digitalRead(BUTTON_LEFT) == LOW){
        if (charx > 9){
            charx = charx - 10;
            refreshkeyboard();
            current_key--;
        }
        else{
            if (page > 1 && charx <= 10 && chary <= 11){
                charx = 70;
                chary = 33;
                page--;
                refreshkeyboard();
                current_key--;
            }
            if (charx <= 10 && chary >= 17){
                charx = 70;
                chary = chary - 8;
                refreshkeyboard();
                current_key--; 
            }
        }
        delay(debouncetime);
        #ifdef debug
        esp8266.print("Charx: ");
        esp8266.println(charx);
        esp8266.print("Chary: ");
        esp8266.println(chary);
        esp8266.print("Current char: ");
        esp8266.println(char(current_key));
        #endif
    }

    if (digitalRead(BUTTON_OK) == LOW){
        // Display the key and save it to the command string
        if(charloc < command.length()){
            // Add a char to the middle of the string
            
            String tmpstring1 = "";
            String tmpstring2 = "";
            
            for(int i = 0; i < charloc; i++){
                tmpstring1.concat(command.charAt(i));
            }

            for(int i = charloc; i < command.length(); i++){
                tmpstring2.concat(command.charAt(i));    
            }

            command = String(tmpstring1 + char(current_key) + tmpstring2); // New string
        }
        else {
            command = String(command + char(current_key));
        }
        
        if(command.length() <= 16){
            endloc++;
        }
        else{
            startloc++;
            endloc++;    
        }
        charloc++;
        
        refreshkeyboard();
        delay(debouncetime);
    }

    if (digitalRead(BUTTON_RIGHT) == LOW){
        if (charx < 70){
            charx = charx + 10;
            refreshkeyboard();
            current_key++;
        }
        else{
            if (page < 4 && charx >= 70 && chary >= 33){
                if (page < 3){
                    charx = 0;
                    chary = 11;
                }
                page++;
                refreshkeyboard();
                current_key++;
            }
            if (charx >= 70 && chary <= 31){
                charx = 0;
                chary = chary + 8;
                current_key++;
                refreshkeyboard();
            }
        }
        delay(debouncetime);
        #ifdef debug
        esp8266.print("Charx: ");
        esp8266.println(charx);
        esp8266.print("Chary: ");
        esp8266.println(chary);
        esp8266.print("Current char: ");
        esp8266.println(char(current_key));
        #endif
    }

    if (digitalRead(BUTTON_UP) == LOW){
        if(chary > 11){
            chary = chary - 8;
            refreshkeyboard();
            current_key = current_key - 8;
        }
        else{
            if(chary <= 11 && page > 1){
                page--;
                chary = 32;
                refreshkeyboard();
                current_key = current_key - 8;
            }
        }
        delay(debouncetime);
        #ifdef debug
        esp8266.print("Charx: ");
        esp8266.println(charx);
        esp8266.print("Chary: ");
        esp8266.println(chary);
        esp8266.print("Current char: ");
        esp8266.println(char(current_key));
        #endif
    }

    if (digitalRead(BUTTON_DOWN) == LOW){
        if(chary < 32){
            chary = chary + 8;
            refreshkeyboard();
            current_key = current_key + 8;
        }
        else{
            if(chary >= 32 && page < 4){
                if (page < 3){
                    chary = 11;
                }
                page++;
                current_key = current_key + 8;
                refreshkeyboard();
            }
        }
        delay(debouncetime);
        #ifdef debug
        esp8266.print("Charx: ");
        esp8266.println(charx);
        esp8266.print("Chary: ");
        esp8266.println(chary);
        esp8266.print("Current char: ");
        esp8266.println(char(current_key));
        #endif
    }
}

void displayterminal(int incomingByte){
    // Displaying the key from esp8266 to terminal
    if (incomingByte == 13){
        termx = 0;
        termy = termy + 8;
    }
    
    else{
        if (!vkeyboard){
            if (termx < 80 && termy < 41){
                lcd.Asc_Char(termx,termy,char(incomingByte),BLACK); // Display the character  
                lcd.Display();
                termx = termx + 5;
            }
            
            else {
                if (termx > 74){
                    // End of line in LCD
                    termx = 5;
                    termy = termy + 8;
                    lcd.Asc_Char(0,termy,char(incomingByte),BLACK); // Display the character
                    lcd.Display();
                }
                
                if (termy > 40){
                    lcd.Clear(); // Clear the LCD screen
                    lcd.Display();
                    termx = 0;
                    termy = 0;
                    esp8266.println(":DOWN"); // Reshow the LCD
                }
            }
        }
        // Else: when the keyboard finish, we will get the data again by using esp8266.println(":DOWN");
    }
}
