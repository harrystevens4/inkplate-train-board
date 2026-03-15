#include <Inkplate.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
//defines WIFI_SSID and WIFI_PASSWORD
#include "wifi_password.h"
//has the api keys
#include "keys.h"

int display_national_rail_departures(const char *crs_code, const char *title);

Inkplate inkplate(INKPLATE_1BIT);

void setup() {
	//====== initialise serial ======
	Serial.begin(115200);
	//====== initialise inkplate ======
	inkplate.begin();
	inkplate.display();
	//====== initialise wifi ======
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	inkplate.clearDisplay();
	inkplate.setTextColor(BLACK);
	inkplate.setCursor(10,150);
	inkplate.setTextSize(5);
	inkplate.print("Connecting to wifi...");
	inkplate.display();
	delay(2000);
	while (WiFi.status() != WL_CONNECTED){
	}
	inkplate.clearDisplay();
	inkplate.setCursor(50,150);
	inkplate.print("Wifi connected.");
	inkplate.display();
	delay(1000);
}

void loop() { 
	display_national_rail_departures("CFB","Cftd Brdg");
	delay(60000); //60s
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
			inkplate.display();
			Serial.println("departures displayed");
		}
	}else {
		Serial.println("could not connect");
		return -1;
	}
	return 0;
}
