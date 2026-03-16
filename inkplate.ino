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
int display_selection_menu(const char **options, size_t option_count);

Inkplate inkplate(INKPLATE_1BIT);

void setup() {
	//====== initialise serial ======
	Serial.begin(115200);
	//====== initialise inkplate ======
	inkplate.begin();
	inkplate.tsInit(true);
	//wake battery monitor up
	inkplate.wakePeripheral(INKPLATE_FUEL_GAUGE);
	inkplate.battery.begin();
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
	//====== update screen ======
	inkplate.display();
	//====== wait for user input ======
	const time_t time_between_updates_ms = 60000;
	time_t time_waited_ms = 0;
	while (time_waited_ms < time_between_updates_ms){
		delay(100);
		time_waited_ms += 100;
		uint16_t x[2], y[2];
		if (inkplate.tsGetData(x,y) > 0){
			selected_menu = (selected_menu+1) % 3;
			break;
		}
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
			//Serial.println(response);
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

int display_selection_menu(const char **options, size_t option_count){
	//====== initialise everything ======
	Serial.println("displaying selection menu");
	inkplate.setTextSize(4);
	inkplate.clearDisplay();
	//constants
	const int button_width = 460;
	const int button_height = 120;
	//====== render option buttons ======
	const int options_per_page = 4;
	int page = 0;
	for (int i = 0; i < option_count; i++){
		int x = 40;
		int y = 50+(i*(button_height+10));
		inkplate.drawRect(x,y,button_width,button_height,BLACK);
		inkplate.setCursor(x+10,y+40);
		inkplate.print(options[i]);
	}
	inkplate.display();
	//====== option buttons touch ======
	while (1){
		for (int i = 0; i < option_count; i++){
			int x = 40;
			int y = 50+(i*(button_height+10));
			if (inkplate.touchInArea(x,y+15,button_width,button_height-30)){
				Serial.println("user selected: "+String(options[i])+" option "+String(i));
				//convey to the user that they selected the option by inverting colours on the selected option
				inkplate.fillRect(x,y,button_width,button_height,BLACK);
				inkplate.setCursor(x+10,y+40);
				inkplate.setTextColor(WHITE);
				inkplate.print(options[i]);
				inkplate.partialUpdate();
				inkplate.setTextColor(BLACK);
				return i;
			}
			delay(100);
		}
	}
}

int wifi_connect(){
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED){
	}
	return 0;
}
