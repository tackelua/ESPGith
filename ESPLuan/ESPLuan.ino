/*
 */


// the setup function runs once when you press reset or power the board
//#define BLYNK_PRINT Serial
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <RCSwitch.h>

#define LED_STATUS	D3
#define RF_PIN		D4

#define RELAY1		0
#define RELAY2		1
#define RELAY3		2
#define RELAY4		3
#define RELAY5		4

#define CONFIG_PRESS		30
#define RESET_PRESS			60
#define CMD_NULL			0
#define CMD_SMARTCONFIG		1
#define CMD_RESET			2

#define LED_SMARTCONFIG		2
#define LED_LOSTWIFI		6

///Remote Button ID
#define REMOTE_A			8206272
#define REMOTE_B			8206128
#define REMOTE_C			8206092
#define REMOTE_D			8206083
#define REMOTE_AB			8206320
#define REMOTE_AC			8206284
#define REMOTE_AD			8206275
#define REMOTE_BC			8206140
#define REMOTE_CD			8206095


char BLYNK_AUTH[] = "1ada1c7eb6c045cca3c6d560db92df4e";//esp luannguyen

BlynkTimer btHardware;

int btStatusID, iBTN = 0, iBTNCmd = CMD_NULL;
int timeline = 0, lLED_S = LED_LOSTWIFI, iRF_Last = 0;

RCSwitch mySwitch = RCSwitch();

int iRL_Pin[5] = { D0, D5, D6, D7, D8 };
bool iRL[5] = {0};
bool bLED = false, bWifiOK = false , bQuitSmartConfig = false;

void setup() {
	hardware_init();

	wifi_init();

	Blynk.config(BLYNK_AUTH, "10.210.6.73", 8442);
	Serial.println(F("Connect to Blynk server"));
	while (!Blynk.connect(10000)) {
		Serial.print(F("."));
		delay(50);
	}
	Serial.println(F("Server Sync..."));

	turn(RELAY1, iRL[RELAY1], V1);
	turn(RELAY2, iRL[RELAY2], V2);
	turn(RELAY3, iRL[RELAY3], V3);
	turn(RELAY4, iRL[RELAY4], V4);
	turn(RELAY5, iRL[RELAY5], V5);
	//Blynk.syncAll();
	Serial.println(F("System Running..."));
}

// the loop function runs over and over again forever
void loop() {          // wait for a second
	if (WiFi.isConnected()) Blynk.run();
	btHardware.run();
	if (WiFi.isConnected()) Blynk.run();
	RF_read();
	if (WiFi.isConnected()) Blynk.run();
	delay(1);
}

void hardware_scan()
{
	if (++timeline > 50000) timeline = 0;
	if (iBTN == 0)
	{
		if (WiFi.isConnected()) digitalWrite(LED_STATUS, LOW);
		else if ((timeline % lLED_S) == 0)
		{
			bLED ^= 1;
			digitalWrite(LED_STATUS, bLED);
		}
		if (iBTNCmd == CMD_SMARTCONFIG) 
			if (lLED_S != LED_SMARTCONFIG) SmartConfig(); 
			else bQuitSmartConfig = true;
		else if (iBTNCmd == CMD_RESET) ESP.reset();
	}
	iBTN = button_read();
	switch (iBTN)
	{
	case 1: digitalWrite(LED_STATUS, HIGH); break;
	case CONFIG_PRESS: digitalWrite(LED_STATUS, LOW); iBTNCmd = CMD_SMARTCONFIG; break;
	case RESET_PRESS: digitalWrite(LED_STATUS, LOW); iBTNCmd = CMD_RESET; break;
	case CONFIG_PRESS + 10:
	case RESET_PRESS + 10:
		digitalWrite(LED_STATUS, HIGH);  iBTNCmd = CMD_NULL; break;
	}
	if ((timeline % 3) == 0) iRF_Last = 0;
}

void hardware_init()
{
	Serial.begin(74880);
	EEPROM.begin(128);
	EEP_Load();
	pinMode(LED_STATUS, OUTPUT);
	pinMode(iRL_Pin[RELAY1], OUTPUT);
	pinMode(iRL_Pin[RELAY2], OUTPUT);
	pinMode(iRL_Pin[RELAY3], OUTPUT);
	pinMode(iRL_Pin[RELAY4], OUTPUT);
	pinMode(iRL_Pin[RELAY5], OUTPUT);
	digitalWrite(iRL_Pin[RELAY1], iRL[RELAY1]);
	digitalWrite(iRL_Pin[RELAY2], iRL[RELAY2]);
	digitalWrite(iRL_Pin[RELAY3], iRL[RELAY3]);
	digitalWrite(iRL_Pin[RELAY4], iRL[RELAY4]);
	digitalWrite(iRL_Pin[RELAY5], iRL[RELAY5]);
	for (int i = 0; i < 6; i++)
	{
		digitalWrite(LED_STATUS, LOW);
		delay(20);
		digitalWrite(LED_STATUS, HIGH);
		delay(20);
	}
	btStatusID = btHardware.setInterval(100, hardware_scan);
	mySwitch.enableReceive(digitalPinToInterrupt(RF_PIN));
}

void wifi_init() {
	bWifiOK = false;
	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);

	WiFi.printDiag(Serial);
	Serial.println(F("\nConnecting to wifi..."));

	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		Serial.println(F("Connected\n"));
		bWifiOK = true;
	}
	else SmartConfig();

	for (int i = 0; i < 4; i++)
	{
		digitalWrite(LED_STATUS, LOW);
		delay(20);
		digitalWrite(LED_STATUS, HIGH);
		delay(20);
	}
}

void SmartConfig()
{
	Serial.println(F("SmartConfig started."));
	bQuitSmartConfig = false;
	lLED_S = LED_SMARTCONFIG;
	iBTNCmd = CMD_NULL;
	WiFi.beginSmartConfig();
	while (1) {
		hardware_scan();
		if (bQuitSmartConfig)
		{
			WiFi.stopSmartConfig();
			WiFi.reconnect();
			lLED_S = LED_LOSTWIFI;
			iBTNCmd = CMD_NULL;
			return;
		}
		RF_read();
		delay(100);
		if (WiFi.smartConfigDone()) {
			Serial.println(F("SmartConfig: Success"));
			WiFi.printDiag(Serial);
			//WiFi.stopSmartConfig();
			lLED_S = LED_LOSTWIFI;
			iBTNCmd = CMD_NULL;
			return;
		}
	}
}

int button_read()
{
	static int btnTime = 0;
	int adc_value = analogRead(A0);
	if (adc_value < 512) btnTime++;
	else btnTime = 0;
	//Serial.print("Button ");
	//Serial.println(btnTime);
	return btnTime;
}

void RF_read()
{
	if (mySwitch.available())
	{
		timeline = 0;
		int value = mySwitch.getReceivedValue();
		if (value == 0) Serial.print(F("Unknown encoding"));
		else
		{
			if (value != iRF_Last)
			{
				Serial.print(F("Received "));
				Serial.println(value);
				iRF_Last = value;
				switch (value)
				{
				case REMOTE_A: iRL[0] ^= 1; turn(RELAY1, iRL[0], V1);
					break;
				case REMOTE_B: iRL[1] ^= 1; turn(RELAY2, iRL[1], V2);
					break;
				case REMOTE_C: iRL[2] ^= 1; turn(RELAY3, iRL[2], V3);
					break;
				case REMOTE_D: iRL[3] ^= 1; turn(RELAY4, iRL[3], V4);
					break;
				case REMOTE_BC: iRL[4] ^= 1; turn(RELAY5, iRL[4], V5);
					break;
				case REMOTE_AD:
					turn(RELAY1, HIGH, V1);
					turn(RELAY2, HIGH, V2);
					turn(RELAY3, HIGH, V3);
					turn(RELAY4, HIGH, V4);
					turn(RELAY5, LOW, V5);
					break;
				default:
					break;
				}
			}

		}
		mySwitch.resetAvailable();
	}
}

BLYNK_WRITE(V1) { iRL[0] = param.asInt(); turn(RELAY1, iRL[0]); }
BLYNK_WRITE(V2) { iRL[1] = param.asInt(); turn(RELAY2, iRL[1]); }
BLYNK_WRITE(V3) { iRL[2] = param.asInt(); turn(RELAY3, iRL[2]); }
BLYNK_WRITE(V4) { iRL[3] = param.asInt(); turn(RELAY4, iRL[3]); }
BLYNK_WRITE(V5) { iRL[4] = param.asInt(); turn(RELAY5, iRL[4]); }
BLYNK_WRITE(V6)
{
	turn(RELAY1, HIGH, V1);
	turn(RELAY2, HIGH, V2);
	turn(RELAY3, HIGH, V3);
	turn(RELAY4, HIGH, V4);
	turn(RELAY5, LOW, V5);
}

void turn(int relay, bool status)
{
	iRL[relay] = status;
	digitalWrite(iRL_Pin[relay], status);
	EEPROM.write(relay, status); EEPROM.commit();
}
void turn(int relay, bool status, int vt)
{
	iRL[relay] = status;
	digitalWrite(iRL_Pin[relay], status);
	Blynk.virtualWrite(vt, status);//if (WiFi.isConnected()) 
	EEPROM.write(relay, status); EEPROM.commit();
}

#define EEP_RELAY1		0
#define EEP_RELAY2		1
#define EEP_RELAY3		2
#define EEP_RELAY4		3
#define EEP_RELAY5		4

void EEP_Load()
{
	iRL[0] = (bool)EEPROM.read(EEP_RELAY1);
	iRL[1] = (bool)EEPROM.read(EEP_RELAY2);
	iRL[2] = (bool)EEPROM.read(EEP_RELAY3);
	iRL[3] = (bool)EEPROM.read(EEP_RELAY4);
	iRL[4] = (bool)EEPROM.read(EEP_RELAY5);
}
