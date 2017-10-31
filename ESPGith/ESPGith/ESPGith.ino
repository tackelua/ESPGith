/*************************************************************
Blynk project.
Open this link and project will be auto created on blynk-cloud
http://tinyurl.com/yd3zle46
*************************************************************/

#pragma region HEADER INCLUDE
/* Comment this out to disable prints and save space */
#include "Button.h"
#include "RCSwitch.h"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <BlynkSimpleEsp8266.h>

#define DEBUG Serial
#define DB(x) DEBUG.println(x)
#define Db(x) DEBUG.print(x) 

#pragma endregion

#pragma region VARIABLE

#define R1		D0
#define R2		D5
#define R3		D6
#define R4		D7
#define R5		D8
#define LED_PIN	D4
#define BUTTON	A0

ESP8266WiFiMulti wifiMulti;

RCSwitch mySwitch = RCSwitch();

//===========================================
#define PULLUP false        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
//switch is closed, this is negative logic, i.e. a high state
//means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.

#define PRESS_FOR_RESET			5000 
#define PRESS_FOR_SMARTCONFIG	2000 
Button myBtn(BUTTON, PULLUP, INVERT, DEBOUNCE_MS);
//===========================================

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).

//server Gith
char BLYNK_AUTH[] = "1ada1c7eb6c045cca3c6d560db92df4e";
char BLYNK_DOMAIN[] = "10.210.6.73";

//server blynk
//char BLYNK_AUTH[] = "8069ddeca7ba4c08b33a98a50ee7d53d";
//char BLYNK_DOMAIN[] = "blynk-cloud.com";

int	 BLYNK_PORT = 8442;
WidgetTerminal Terminal(V0);

#pragma endregion

//=======================================

#pragma region LED CONTROLLING
typedef enum {
	ON = 1,
	OFF = 2,
	BLINK = 3
} led_mode_t;

#define LED_ON	LOW
#define LED_OFF	HIGH

#define LED_INTERVAL_SMARTCONFIG	100
#define LED_INTERVAL_NO_WIFI		1000


bool ledInit() {
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LED_OFF);
	return LED_OFF;
}

static bool led_stt = ledInit();
void ledOut(bool status) {
	if (led_stt != status) {
		led_stt = status;
		digitalWrite(LED_PIN, led_stt);
	}
}

void led_run(int mode, unsigned long interval = 1000) {
	if (mode == ON) {
		ledOut(LED_ON);
	}
	else if (mode == OFF) {
		ledOut(LED_OFF);
	}
	else if (mode == BLINK) {
		static unsigned long t = millis();
		if ((millis() - t) >= interval) {
			t = millis();
			ledOut(!led_stt);
		}
	}
}
#pragma endregion

//=======================================

#pragma region BUTTON CONTROLLING
void myBtn_run() {
	myBtn.read();

	if (myBtn.pressedFor(PRESS_FOR_SMARTCONFIG)) {
		//DB("Press long");
		wifi_smartConfig();
	}
}
#pragma endregion


//=======================================

#pragma region WIFI CONFIG
typedef enum {
	NORMAL = 0,
	SMARTCONFIG = 1,
	WIFIMULTI = 2
} WiFi_Setup_Mode_t;
void wifi_smartConfig() {
	Serial.println(F("\r\n# SmartConfig started."));
	WiFi.beginSmartConfig();
	//DB(F("Hi, you can enter wifi info via Serial. <SSID>|<Password>\r\nExample: GithAP|giathinh123"));
	while (1) {
		//delay(500);
		delay(100);

		led_run(BLINK, LED_INTERVAL_SMARTCONFIG);
		RCSwitch_run();

		//check button to reset if press again
		{
			myBtn.read();
			if (myBtn.pressedFor(PRESS_FOR_RESET)) {
				//DB(F("Reset"));
				delay(100);
				digitalWrite(0, HIGH);
				digitalWrite(2, HIGH);
				digitalWrite(15, LOW);
				delay(1000);
				//ESP.reset();
				pinMode(0, INPUT);
				pinMode(2, INPUT);
				pinMode(15, INPUT);
				delay(200);
				ESP.restart();
				delay(1000);
			}
		}

		if (WiFi.smartConfigDone()) {
			Serial.println(F("SmartConfig: Success"));
			WiFi.printDiag(Serial);
			if (WiFi.waitForConnectResult() == WL_CONNECTED) {
				//DB(F("connected"));
			}
			WiFi.stopSmartConfig();
			break;
		}

		
		if (Serial.available()) {
			String wifiInput = Serial.readString();
			wifiInput.trim();
			int pos = wifiInput.indexOf("|");
			if (pos > 0) {
				String _ssid = wifiInput.substring(0, pos);
				String _pw = wifiInput.substring(pos + 1);
				WiFi.disconnect();
				delay(100);
				WiFi.begin(_ssid.c_str(), _pw.c_str());
				//Db(F("Connecting to "));
				//DB(_ssid);
				//Db(F("Password: "));
				//DB(_pw);
				////DB(F("If connect success, ok. If fail, you must reset the board manually :D"));
				if (WiFi.waitForConnectResult() == WL_CONNECTED) {
					//DB(F("Connected"""));
				}
				else {
					//DB(F("Reset"));
					ESP.restart();
					delay(1000);
				}
				//while (1) {
				//	if (WiFi.isConnected()) {
				//		return;
				//	}
				//	else {
				//		delay(10);
				//		led_run(BLINK, LED_INTERVAL_SMARTCONFIG);
				//	}
				//}
			}
			else {
				//DB(F("Syntax error, must have '|' between SSID and Password"));
			}
		}
		
	}
}
void wifi_init(int mode) {
	//DB();
	WiFi.printDiag(Serial);
	//DB(F("\r\n# WiFi init"));

	WiFi.setAutoConnect(true);
	WiFi.setAutoReconnect(true);
	WiFi.mode(WIFI_STA);

	if (WiFi.waitForConnectResult() == WL_CONNECTED)
	{
		//DB(F("connected\n"));
	}
	else {
		//DB(F("auto connect fail. Press button to config wifi"));
	}

	if (mode == NORMAL)
	{
		while (1) {
			delay(1);
			if (WiFi.isConnected()) {
				led_run(ON);
				break;
			}
			else {
				RCSwitch_run();
				led_run(BLINK, LED_INTERVAL_NO_WIFI);
				myBtn_run();
			}
		}
	}

	else if (mode == SMARTCONFIG)
	{
		WiFi.printDiag(Serial);
		if (WiFi.waitForConnectResult() == WL_CONNECTED)
		{
			Serial.println(F("connected\n"));
		}
		else
		{
			wifi_smartConfig();
		}
	}

	else if (mode = WIFIMULTI)
	{
		wifiMulti.addAP("Nokia 6", "mic12345678");
		wifiMulti.addAP("GithAP", "giathinh123");
		wifiMulti.addAP("Dingtea T2_sau", "133nguyenvanlinh");

		if (wifiMulti.run() == WL_CONNECTED) {
			Serial.println("");
			Serial.println("WiFi connected");
			Serial.println("IP address: ");
			Serial.println(WiFi.localIP());
		}
	}
}
#pragma endregion

//=======================================

#pragma region BLYNK CONTROLLING
BLYNK_WRITE(V1) {
	int buttonStt = param.asInt();
	digitalWrite(R1, buttonStt);
	//Blynk.virtualWrite(V11, buttonStt);
	//Db(F("#V1: "));
	//DB(buttonStt);
	Terminal.print("#V1: ");
	Terminal.println(buttonStt);
	Terminal.flush();
}
BLYNK_WRITE(V2) {
	int buttonStt = param.asInt();
	digitalWrite(R2, buttonStt);
	//Blynk.virtualWrite(V12, buttonStt);
	//Db(F("#V2: "));
	//DB(buttonStt);
	Terminal.print("#V2: ");
	Terminal.println(buttonStt);
	Terminal.flush();
}
BLYNK_WRITE(V3) {
	int buttonStt = param.asInt();
	digitalWrite(R3, buttonStt);
	//Blynk.virtualWrite(V13, buttonStt);
	//Db(F("#V3: "));
	//DB(buttonStt);
	Terminal.print("#V3: ");
	Terminal.println(buttonStt);
	Terminal.flush();
}
BLYNK_WRITE(V4) {
	int buttonStt = param.asInt();
	digitalWrite(R4, buttonStt);
	//Blynk.virtualWrite(V14, buttonStt);
	//Db(F("#V4: "));
	//DB(buttonStt);
	Terminal.print("#V4: ");
	Terminal.println(buttonStt);
	Terminal.flush();
}
BLYNK_WRITE(V5) {
	int buttonStt = param.asInt();
	digitalWrite(R5, !buttonStt);
	//Blynk.virtualWrite(V15, !buttonStt);
	//Db(F("#V5: "));
	//DB(!buttonStt);
	Terminal.print("#V5: ");
	Terminal.println(!buttonStt);
	Terminal.flush();
}
BLYNK_CONNECTED() {
	Blynk.syncAll();
}

BLYNK_WRITE(V6) {
	int buttonStt = param.asInt();
	digitalWrite(R1, buttonStt);
	digitalWrite(R2, buttonStt);
	digitalWrite(R3, buttonStt);
	digitalWrite(R4, buttonStt);
	digitalWrite(R5, !buttonStt);
	//Blynk.virtualWrite(V11, buttonStt);
	//Blynk.virtualWrite(V12, buttonStt);
	//Blynk.virtualWrite(V13, buttonStt);
	//Blynk.virtualWrite(V14, buttonStt);
	//Blynk.virtualWrite(V15, !buttonStt);
	Blynk.virtualWrite(V1, buttonStt);
	Blynk.virtualWrite(V2, buttonStt);
	Blynk.virtualWrite(V3, buttonStt);
	Blynk.virtualWrite(V4, buttonStt);
	Blynk.virtualWrite(V5, buttonStt);
	//Db(F("#V6: "));
	//DB(buttonStt);
	Terminal.print("#V6: ");
	Terminal.println(buttonStt);
	Terminal.flush();
}

#pragma endregion

//=======================================

#pragma region RC SWITCH CONTROLLING
void RCSwitch_run() {
	static ulong t = millis();
	if (millis() - t > 100) {
		t = millis();
		if (mySwitch.available()) {
			int value = mySwitch.getReceivedValue();
			//DB(value);
			if (value == 8206272) { //A
				bool stt = digitalRead(R1);
				digitalWrite(R1, !stt);
				if (WiFi.isConnected()) {
					Blynk.virtualWrite(V1, !stt);	Blynk.run();
				}
			}
			else if (value == 8206128) { //B
				bool stt = digitalRead(R2);
				digitalWrite(R2, !stt);
				if (WiFi.isConnected()) {
					Blynk.virtualWrite(V2, !stt);	Blynk.run();
				}
			}
			else if (value == 8206092) { //C
				bool stt = digitalRead(R3);
				digitalWrite(R3, !stt);
				if (WiFi.isConnected()) {
					Blynk.virtualWrite(V3, !stt);	Blynk.run();
				}
			}
			else if (value == 8206083) { //D
				bool stt = digitalRead(R4);
				digitalWrite(R4, !stt);
				if (WiFi.isConnected()) {
					Blynk.virtualWrite(V4, !stt);	Blynk.run();
				}
			}
			else if (value == 8206284) { //AC
				bool stt = digitalRead(R5);
				digitalWrite(R5, !stt);
				if (WiFi.isConnected()) {
					Blynk.virtualWrite(V5, !stt);	Blynk.run();
				}
			}
			else if (value == 8206275) { //AD
				bool stt1 = digitalRead(R1);
				bool stt2 = digitalRead(R2);
				bool stt3 = digitalRead(R3);
				bool stt4 = digitalRead(R4);
				bool stt5 = digitalRead(R5);
				digitalWrite(R1, !stt1);
				digitalWrite(R2, !stt2);
				digitalWrite(R3, !stt3);
				digitalWrite(R4, !stt4);
				digitalWrite(R5, !stt5);

				if (WiFi.isConnected()) {
					Blynk.run();
					Blynk.virtualWrite(V1, !stt1);	Blynk.run();
					Blynk.virtualWrite(V2, !stt2);	Blynk.run();
					Blynk.virtualWrite(V3, !stt3);	Blynk.run();
					Blynk.virtualWrite(V4, !stt4);	Blynk.run();
					Blynk.virtualWrite(V5, !stt5);	Blynk.run();
				}
			}

			mySwitch.resetAvailable();
		}
	}
}
#pragma endregion

//=======================================

#pragma region PROGRAM
void setup()
{
	delay(100);
	Serial.begin(74880);
	Serial.setTimeout(50);

	pinMode(R1, OUTPUT);
	pinMode(R2, OUTPUT);
	pinMode(R3, OUTPUT);
	pinMode(R4, OUTPUT);
	pinMode(R5, OUTPUT);
	ledOut(LED_OFF);

	mySwitch.enableReceive(digitalPinToInterrupt(D3));

	//WiFi.begin("GithAP", "giathinh123");
	wifi_init(NORMAL);

	//Blynk.config(BLYNK_AUTH, "blynk-cloud.com", 8442);
	Blynk.config(BLYNK_AUTH, BLYNK_DOMAIN, BLYNK_PORT);
	Serial.println(F("Connect to Blynk server"));

	Blynk.connect();
	//while ((wifiMulti.run() != WL_CONNECTED) && (!Blynk.connect())) {
	//	Serial.print(F("."));
	//	delay(50);
	//}

	Terminal.println(BLYNK_AUTH);
}

void loop()
{
	//if (wifiMulti.run() != WL_CONNECTED) {
	//	delay(1);
	//	return;
	//}
	//ulong t = millis();

	static bool loop1st = false;
	if (!loop1st) {
		loop1st = true;
		//DB("Loop running");
	}
	if (WiFi.isConnected()) {
		led_run(ON);
	}
	else {
		led_run(BLINK, LED_INTERVAL_NO_WIFI);
	}

	Blynk.run();
	RCSwitch_run();
	Blynk.run();
	myBtn_run();
	Blynk.run();
	delay(1);
	////DB(millis() - t);
}
#pragma endregion


