#include <Inkplate.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
//defines WIFI_SSID and WIFI_PASSWORD
#include "wifi_password.h"
//has the api keys
#include "keys.h"

int display_national_rail_departures(const char *crs_code, const char *title);
int wifi_connect();

Inkplate inkplate(INKPLATE_1BIT);

void setup() {
	//====== initialise serial ======
	Serial.begin(115200);
	//====== initialise inkplate ======
	inkplate.begin();
	//====== initialise wifi ======
	wifi_connect();
}

void loop() { 
	//====== display menus ======
	static int selected_menu = 0;
	switch (selected_menu){
	case 0:
		display_national_rail_departures("CFB","Cftd Brdg");
		break;
	case 1:
		display_national_rail_departures("CTF","Catford");
		break;
	case 2:
		display_national_rail_departures("HPA","Hnr Oak Pk");
		break;
	}
	//====== display batter info ======
	//wake battery
	inkplate.wakePeripheral(INKPLATE_FUEL_GAUGE);
	inkplate.battery.begin();
	//get charge
	int percentage_charge = inkplate.battery.soc();
	//get power draw
	int current_draw = inkplate.battery.current();
	//display
	inkplate.setTextColor(BLACK);
	inkplate.setCursor(30,530);
	inkplate.setTextSize(3);
	inkplate.print("Bat: "+String(percentage_charge)+"%");
	//power off indefinitely if battery goes below 5%
	if (percentage_charge < 5 && current_draw > 0){
		inkplate.clearDisplay();
		inkplate.setTextColor(BLACK);
		inkplate.setCursor(50,50);
		inkplate.setTextSize(4);
		inkplate.print("low battery");
		inkplate.display();
		esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
		esp_deep_sleep_start();
	}
	//====== update screen ======
	inkplate.display();
	//====== sleep ======
	Serial.println("sleeping...");
	esp_sleep_enable_timer_wakeup(60000000); //60s timer
	esp_sleep_enable_ext0_wakeup(GPIO_NUM_36,LOW); //or wake button pressed
	//save power
	WiFi.disconnect();
	esp_light_sleep_start();
	//====== wakeup ======
	//bring wifi back up after light sleep exit
	wifi_connect();
	//why did we wake?
	esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
	Serial.println("woke up from source "+String(wakeup_cause));
	if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT0){
		Serial.print("changing menu ("+String(selected_menu)+" -> ");
		selected_menu = (selected_menu+1) % 3;
		Serial.println(String(selected_menu)+" )");
	}
}

int display_national_rail_departures(const char *crs_code, const char *title){
	//====== fetch train times ======
	Serial.println("requesting departures from "+String(title)+"...");
	HTTPClient http;
	if (http.begin("https://api1.raildata.org.uk/1010-live-departure-board-dep1_2/LDBWS/api/20220120/GetDepartureBoard/"+String(crs_code))){
		http.addHeader("x-apikey",LDBWS_DEPARTURES_KEY);
		int http_response_code = http.GET();
		Serial.println("got response code "+String(http_response_code));
		if (http_response_code == 200){
			//====== deserialise =====
			String response = http.getString();
			Serial.println(response);
			JsonDocument json_response;
			deserializeJson(json_response, response);
			//====== display header ======
			inkplate.clearDisplay();
			inkplate.setTextColor(BLACK);
			inkplate.setCursor(50,50);
			inkplate.setTextSize(4);
			inkplate.print(String(title)+" Departures");
			inkplate.fillRect(50,90,500,10,BLACK);
			//====== display each train time ======
			inkplate.setTextSize(3);
			int i = 0;
			JsonArray services = json_response["trainServices"];
			if (services.isNull()){
				Serial.println("no services provided");
				return -1;
			}
			Serial.println("train times fetched successfully");
			for (JsonVariant departure : services){
				String stated_time_of_departure = departure["std"] | "N/A";
				JsonArray destinations = departure["destination"];
				//grab the first destination
				String destination = "No dest";
				if (destinations.size() > 0){
					destination = destinations[0]["locationName"] | "N/A";
				}
				inkplate.setCursor(40,130+(35*i));
				inkplate.print(destination);
				inkplate.setCursor(475,130+(35*i));
				inkplate.print(stated_time_of_departure);
				i++;
			}
			//====== display ======
			Serial.println("departures displayed");
		}
	}else {
		Serial.println("could not connect");
		return -1;
	}
	return 0;
}

int wifi_connect(){
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED){
	}
	return 0;
}
