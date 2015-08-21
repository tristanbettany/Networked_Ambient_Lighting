#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_WS2801_Ramped.h>

/**
 * Setup LED strip
 */
int dataPin  = 2;
int clockPin = 3;
int LEDcount = 20;

byte cmd, prevCmd, setOff;
byte globalRed, globalGreen, globalBlue;

Adafruit_WS2801 strip = Adafruit_WS2801(LEDcount, dataPin, clockPin);

/**
 * Serial method to wait for bytes
 */
void WaitForBytes(int count)
{
	while (Serial.available() < count) {}
}

/**
 * Setup ethernet
 */
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 7, 253 };
byte gateway[] = { 192, 168, 6, 1 };
byte subnet[] = { 255, 255, 254, 0 };

EthernetServer server = EthernetServer(999);
EthernetClient client;

//Packet Start and End Booleans
bool packetStart, packetEnd;
//Variable to temporarily store each character read from Ethernet
char packetData;
//Array to house the bytes from the packet
byte packetArray[32];
//Array Index Number
byte packetArrayIndex;

/**
 * Method runs on boot of arduino
 */
void setup()
{
	//Start LED Strip
	strip.begin();
	//Show LED Strip
	strip.show();
	//Open Serial Port
	Serial.begin(9600);
	//Boot Sequence (Fade In and Out Cobalt)
	fadeInAndOut(0,255,255);
	//Make Sure LEDs Are Off
	cmd = 0;
	//Open Ethernet
	Ethernet.begin(mac, ip, gateway, subnet);
	//Start Server
	server.begin();
}

/**
 * The Main Loop
 */
void loop()
{
	//Listen for incoming connections
	client = server.available();

	if (client)
	{
		//Check if Client is connected
		while (client.connected() && client.available() > 0)
		{
			//Read First Character sent from client
			packetData = client.read();
			if(packetData == '<')
		  {
				//If it found the < char then packet has begun
				packetArrayIndex = 0;
				packetStart = true;
				packetEnd = false;
		  }
			else if (packetData == '>')
			{
				//If it found the > char then packet has ended
				packetArray[packetArrayIndex] = byte('\0');
				packetEnd = true;
				break;
			}
			else
			{
				if (packetArrayIndex < 32)
				{
					//All other cases store the data as a byte in an array of bytes
					packetArray[packetArrayIndex] = byte(packetData);
					packetArrayIndex++;
				}
			}
		}
	}

	if (packetStart && packetEnd)
	{
		//Allow a function to start with LEDs Off
		setOff = 0;
		//Log Previous Command
		prevCmd = cmd;
		//Command sets to first byte in packet array
		cmd = packetArray[0];
		//Reset vairiables to allow new packet to arrive
		packetStart = false;
		packetEnd = false;
		packetArrayIndex = 0;
	}

	if (Serial.available() >= 1)
	{
		//Allow a function to start with LEDs Off
		setOff = 0;
		//Log Previous Command
		prevCmd = cmd;
		//Read Command From Serial
		cmd = Serial.read();
	}

	switch (cmd)
	{
		//Turn All LEDs Off
    case 0:
			allOff();
			break;
		//Alert with custom color a set amount of times
		case 10:
			alert();
			cmd = prevCmd;
			break;
		//Cross Fade between 2 Custom Colors
		case 11:
			crossFadeAlert();
			cmd = prevCmd;
			break;
		//Expand and shrink from centre pixel with custom color a set amount of times
		case 12:
			expandAlert();
			cmd = prevCmd;
			break;
		//Alert Flashing a cutom amount of times with custom color and delays
		case 13:
			flasherAlert();
			cmd = prevCmd;
			break;
		//Cycle a hard coded set of colors
		case 50:
			colorCycle();
			break;
		//A moving raindow spread across the pixels
		case 51:
			rainbow(10);
			break;
		//A solid White light
		case 52:
			lightMode();
			break;
		//A Simulation of police car lights
		case 53:
			emergency();
			break;
		//Custom Coloured Solid light
    case 54:
      colouredLightMode();
      break;
		//An equalizer
    case 55:
      eq();
      break;
		default:
		break;
	}
}

/**
 * Equalizer
 */
void eq()
{
  byte level;
  if (Serial.available() >= 1)
	{
    //Read command options from Serial
    WaitForBytes(1);
    level = Serial.read();
  }
	else
	{
    //Read Command options from Ethernet
    level = packetArray[1];
  }

	int i;
  for (i=level+1; i < 20; i++)
	{
    strip.setPixelColor(i, 0, 0, 0);
  }

	for (i=0; i <= level; i++)
	{
    if (i >= 17)
		{
    	strip.setPixelColor(i, 255, 0, 0);
    }
		else if (i >= 14)
		{
    	strip.setPixelColor(i, 255, 255, 0);
    }
		else
		{
    	strip.setPixelColor(i, 0, 255, 0);
    }
  }

	strip.show();
}

/**
 * Alert flashing a custom amount of times with a custom colour and delays
 */
void flasherAlert()
{
	byte red, green, blue, count, delayOff, delayOn;

	//If Previous Command is Light Mode - Fade Out Gracefully
	if (prevCmd == 52)
	{
		fadeOutAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeOutAll(globalRed,globalGreen,globalBlue);
	}
	else
	{
		colorAll(Color(0,0,0));
	}

	if (Serial.available() >= 1)
	{
		//Read command options from Serial
		WaitForBytes(5);
		red = Serial.read();
		green = Serial.read();
		blue = Serial.read();
		count = Serial.read();
		delayOff = Serial.read();
		delayOn = Serial.read();
	}
	else
	{
		//Read Command options from Ethernet
		red = packetArray[1];
		green = packetArray[2];
		blue = packetArray[3];
		count = packetArray[4];
		delayOff = packetArray[5];
		delayOn = packetArray[6];
	}

	//Do Alert
	int i;
	for (i=1; i <= count; i++)
	{
		colorAll(Color(red,green,blue));
		delay(delayOn * 100);
		colorAll(Color(0,0,0));
		delay(delayOff * 100);
	}

	//If Previous Command is Light Mode - Fade Back in Gracefully
	if (prevCmd == 52)
	{
		fadeInAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeInAll(globalRed,globalGreen,globalBlue);
	}
}

/**
 * Alert user by fading a custom colour in and out a custom amount of times
 */
void alert()
{
	byte red, green, blue, count;

	//If Previous Command is Light Mode - Fade Out Gracefully
	if (prevCmd == 52)
	{
		fadeOutAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeOutAll(globalRed,globalGreen,globalBlue);
	}
	else
	{
		colorAll(Color(0,0,0));
	}

	if (Serial.available() >= 1)
	{
		//Read command options from Serial
		WaitForBytes(4);
		red = Serial.read();
		green = Serial.read();
		blue = Serial.read();
		count = Serial.read();
	}
	else
	{
		//Read Command options from Ethernet
		red = packetArray[1];
		green = packetArray[2];
		blue = packetArray[3];
		count = packetArray[4];
	}

	//Do Alert
	int i;
	for (i=1; i <= count; i++)
	{
		fadeInAndOut(red,green,blue);
	}

	//If Previous Command is Light Mode - Fade Back in Gracefully
	if (prevCmd == 52)
	{
		fadeInAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeInAll(globalRed,globalGreen,globalBlue);
	}
}

/**
 * Alert user by fading in a custom colour and then crossfading to another custom colour before fading out again
 */
void crossFadeAlert()
{
	byte red1, green1, blue1, red2, green2, blue2;

	//If Previous Command is Light Mode - Fade Out Gracefully
	if (prevCmd == 52)
	{
		fadeOutAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeOutAll(globalRed,globalGreen,globalBlue);
	}
	else
	{
		colorAll(Color(0,0,0));
	}

	if (Serial.available() >= 1)
	{
		//Read command options from Serial
		WaitForBytes(6);
		red1 = Serial.read();
		green1 = Serial.read();
		blue1 = Serial.read();
		red2 = Serial.read();
		green2 = Serial.read();
		blue2 = Serial.read();
	}
	else
	{
		//Read Command options from Ethernet
		red1 = packetArray[1];
		green1 = packetArray[2];
		blue1 = packetArray[3];
		red2 = packetArray[4];
		green2 = packetArray[5];
		blue2 = packetArray[6];
	}

	//Do Alert
	fadeInAll(red1,green1,blue1);
	crossFade(red1,green1,blue1,red2,green2,blue2);
	fadeOutAll(red2,green2,blue2);

	//If Previous Command is Light Mode - Fade Back in Gracefully
	if (prevCmd == 52)
	{
		fadeInAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeInAll(globalRed,globalGreen,globalBlue);
	}
}

/**
 * Alert user by filling pixels from centre out towards both edges of the strip with colour white and then retreat back to centre
 */
void expandAlert()
{
	byte red, green, blue, count;

	//If Previous Command is Light Mode - Fade Out Gracefully
	if (prevCmd == 52)
	{
		fadeOutAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeOutAll(globalRed,globalGreen,globalBlue);
	}
	else
	{
		colorAll(Color(0,0,0));
	}

	if (Serial.available() >= 1)
	{
		//Read command options from Serial
		WaitForBytes(4);
		red = Serial.read();
		green = Serial.read();
		blue = Serial.read();
		count = Serial.read();
	}
	else
	{
		//Read Command options from Ethernet
		red = packetArray[1];
		green = packetArray[2];
		blue = packetArray[3];
		count = packetArray[4];
	}

	int i, c;
	for (c=1; c <= count; c++)
	{
		//Expand From Centre Pixel
		for (i=0; i <= 10; i++)
		{
			strip.setPixelColor(9-i, Color(red,green,blue));
			strip.setPixelColor(10+i, Color(red,green,blue));
			strip.show();
			delay(40);
		}

		delay(200);

		//Shrink Back to Centre Pixel
		for (i=0; i <= 10; i++)
		{
			strip.setPixelColor(19-i, Color(0,0,0));
			strip.setPixelColor(0+i, Color(0,0,0));
			strip.show();
			delay(40);
		}
	}

	//If Previous Command is Light Mode - Fade Back in Gracefully
	if (prevCmd == 52)
	{
		fadeInAll(255,255,255);
	}
	else if (prevCmd == 54)
	{
		fadeInAll(globalRed,globalGreen,globalBlue);
	}
}

/**
 * Run through a series of colours filling the strand one ofter the other
 */
void colorCycle()
{
	//Color Cycleing Sequence
	colorWipe(Color(255, 255, 255), 50);
	colorWipe(Color(255, 0, 255), 50);
	colorWipe(Color(255, 0, 0), 50);
	colorWipe(Color(0, 0, 255), 50);
	colorWipe(Color(0, 255, 255), 50);
	colorWipe(Color(255, 255, 0), 50);
	colorWipe(Color(0, 255, 0), 50);
}

/**
 * I can see a rainbow, see a rainbow, see a rainbow too
 */
void rainbow(uint8_t wait)
{
	int i, j, k;
	for (j=0; j < 256; j++)
	{
		for (i=0; i < strip.numPixels(); i++)
		{
			strip.setPixelColor(i, Wheel( ((i * 64 / 5) + j) % 256) );
		}
		strip.show();
		delay(wait);
	}
}

/**
 * Solid white light
 */
void lightMode()
{
	colorAll(Color(255,255,255));
}

/**
 * Coloured light
 */
void colouredLightMode()
{
	if (Serial.available() >= 1)
	{
		//Read command options from Serial
		WaitForBytes(3);
		globalRed = Serial.read();
		globalGreen = Serial.read();
		globalBlue = Serial.read();
	}
	else
	{
		//Read Command options from Ethernet
		globalRed = packetArray[1];
		globalGreen = packetArray[2];
		globalBlue = packetArray[3];
	}

	colorAll(Color(globalRed,globalGreen,globalBlue));
}

/**
 * Immitates emergency services flashing lights
 */
void emergency()
{
	//Make sure LEDs turn off the first time this function is ran inside the loop
	if (setOff == 0)
	{
		colorAll(Color(0,0,0));
		setOff = 1;
	}

	int i, x;
	for (x=0; x < 4; x++)
	{
		for (i=0; i < 10; i++)
		{
			strip.setPixelColor(i, Color(0,0,255));
		}
		strip.show();
		delay(80);
		for (i=0; i < 10; i++)
		{
			strip.setPixelColor(i, Color(0,0,0));
		}
		strip.show();
		delay(80);
	}

	for (x=0; x < 4; x++)
	{
		for (i=10; i < 20; i++)
		{
			strip.setPixelColor(i, Color(255,0,0));
		}
		strip.show();
		delay(80);
		for (i=10; i < 20; i++)
		{
			strip.setPixelColor(i, Color(0,0,0));
		}
		strip.show();
		delay(80);
	}
}

/**
 * Turn all the LEDs off
 */
void allOff()
{
	colorAll(Color(0,0,0));
}

 /**
  * Fade all LEDs in and out with a set colour
  */
void fadeInAndOut(int r, int g, int b)
{
	fadeInAll(r,g,b);
	delay(500);
	fadeOutAll(r,g,b);
	delay(125);
}

/**
 * Fade all LEDs into a colour
 */
void fadeInAll(int targetR, int targetG, int targetB)
{
	colorAll(Color(0,0,0));
	int r = 0;
	int g = 0;
	int b = 0;

	for (int i = 0; i < 256; i++)
	{
		r += targetR;
		g += targetG;
		b += targetB;
		colorAll(Color(r>>8,g>>8,b>>8));
	}
}

/**
 * Fade all LEDs out of a set colour
 */
void fadeOutAll(int targetR, int targetG, int targetB)
{
	colorAll(Color(targetR,targetG,targetB));
	int r = targetR * 256;
	int g = targetG * 256;
	int b = targetB * 256;

	for (int i = 0; i < 256; i++)
	{
		r -= targetR;
		g -= targetG;
		b -= targetB;
		colorAll(Color(r>>8,g>>8,b>>8));
	}
}

/**
 * Crossfade from one colour to another
 */
void crossFade(int r1, int g1, int b1, int r2, int g2, int b2)
{
	colorAll(Color(r1,g1,b1));
	int r = r1 * 256, rD = r2-r1;
	int g = g1 * 256, gD = g2-g1;
	int b = b1 * 256, bD = b2-b1;

	for (int i = 0; i < 256; i++)
	{
		r += rD;
		g += gD;
		b += bD;
		colorAll(Color(r>>8,g>>8,b>>8));
	}
}

/**
 * Set all LEDs one specific colour
 */
void colorAll(uint32_t c)
{
	int i;
	for (i=0; i < strip.numPixels(); i++)
	{
		strip.setPixelColor(i, c);
	}

	strip.show();
}

/**
 * Colour Wipe across the strand
 */
void colorWipe(uint32_t c, uint8_t wait)
{
	int i;
	for (i=0; i < strip.numPixels(); i++)
	{
		strip.setPixelColor(i, c);
		strip.show();
		delay(wait);
	}
}

/**
 * The Colour Wheel
 */
const uint8_t wheelData[][3] = {{255,0,0},{255,3,0},{255,7,0},{255,10,0},{255,14,0},{255,17,0},{255,21,0},{255,24,0},{255,28,0},{255,31,0},{255,35,0},{255,38,0},{255,42,0},{255,45,0},{255,49,0},{255,52,0},{255,56,0},{255,59,0},{255,63,0},{255,66,0},{255,70,0},{255,73,0},{255,77,0},{255,80,0},{255,84,0},{255,87,0},{255,91,0},{255,94,0},{255,98,0},{255,101,0},{255,105,0},{255,108,0},{255,112,0},{255,115,0},{255,119,0},{255,122,0},{255,126,0},{255,129,0},{255,132,0},{255,136,0},{255,139,0},{255,143,0},{255,146,0},{255,150,0},{255,153,0},{255,157,0},{255,160,0},{255,164,0},{255,167,0},{255,171,0},{255,174,0},{255,178,0},{255,181,0},{255,185,0},{255,188,0},{255,191,0},{255,195,0},{255,198,0},{255,202,0},{255,205,0},{255,209,0},{255,212,0},{255,216,0},{255,219,0},{255,223,0},{255,226,0},{255,230,0},{255,233,0},{255,237,0},{255,240,0},{255,244,0},{255,247,0},{255,251,0},{255,254,0},{249,255,0},{242,255,0},{235,255,0},{228,255,0},{221,255,0},{214,255,0},{207,255,0},{200,255,0},{193,255,0},{ 186,255,0},{179,255,0},{172,255,0},{165,255,0},{158,255,0},{151,255,0},{144,255,0},{137,255,0},{130,255,0},{123,255,0},{116,255,0},{109,255,0},{102,255,0},{95,255,0},{88,255,0},{81,255,0},{74,255,0},{67,255,0},{60,255,0},{53,255,0},{46,255,0},{39,255,0},{32,255,0},{25,255,0},{18,255,0},{11,255,0},{4,255,0},{0,253,1},{0,246,8},{0,239,15},{0,232,22},{0,225,29},{0,218,36},{0,211,43},{0,204,50},{0,197,57},{0,190,64},{0,183,71},{0,176,78},{0,169,85},{0,162,92},{0,155,99},{0,148,106},{0,141,113},{0,134,120},{0,127,127},{0,120,134},{0,113,141},{0,106,148},{0,99,155},{0,92,162},{0,85,169},{0,78,176},{0,71,183},{0,64,190},{0,57,197},{0,50,204},{0,43,211},{0,36,218},{0,29,225},{0,22,232},{0,15,239},{0,8,246},{0,1,253},{1,0,252},{3,0,249},{5,0,245},{7,0,242},{9,0,238},{11,0,235},{13,0,232},{15,0,228},{17,0,225},{19,0,221},{21,0,218},{24,0,214},{26,0,211},{28,0,208},{30,0,204},{32,0,201},{34,0,197},{36,0,194},{38,0,191},{40,0,187},{42,0,184},{44,0,180},{46,0,177},{ 48,0,173},{50,0,170},{52,0,167},{54,0,163},{56,0,160},{58,0,156},{60,0,153},{62,0,150},{65,0,146},{67,0,143},{69,0,139},{71,0,136},{73,0,132},{75,0,130},{77,0,132},{79,0,134},{81,0,136},{83,0,139},{85,0,141},{87,0,143},{89,0,145},{91,0,148},{93,0,150},{95,0,152},{97,0,154},{99,0,156},{101,0,159},{103,0,161},{105,0,163},{107,0,165},{109,0,167},{111,0,170},{113,0,172},{115,0,174},{117,0,176},{119,0,179},{121,0,181},{123,0,183},{125,0,185},{127,0,187},{129,0,190},{131,0,192},{133,0,194},{135,0,196},{137,0,198},{139,0,201},{141,0,203},{143,0,205},{145,0,207},{147,0,210},{149,0,207},{152,0,201},{155,0,196},{158,0,190},{161,0,184},{164,0,178},{167,0,173},{170,0,167},{173,0,161},{176,0,155},{178,0,150},{181,0,144},{184,0,138},{187,0,132},{190,0,126},{193,0,121},{196,0,115},{199,0,109},{202,0,103},{205,0,98},{208,0,92},{211,0,86},{214,0,80},{216,0,75},{219,0,69},{222,0,63},{225,0,57},{228,0,51},{231,0,46},{234,0,40},{237,0,34},{240,0,28},{243,0,23},{246,0,17},{ 249,0,11},{252,0,5}};

uint32_t Wheel(byte pos)
{
	return Color(wheelData[pos][0], wheelData[pos][1], wheelData[pos][2]);
}

/**
 * RGB conversion
 */
uint32_t Color(byte g, byte r, byte b)
{
	uint32_t c;
	c = r;
	c <<= 8;
	c |= g;
	c <<= 8;
	c |= b;

	return c;
}
