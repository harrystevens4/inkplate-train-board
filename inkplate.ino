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
void display_title(const char *title);

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
	//====== initialisation ======
	static char *titles[] = {
		"Catford Bridge",
		"Catford",
		"Hnr Oak Park",
		"284 Arrivals"
	};
	static int selected_menu = -1;
	if (selected_menu < 0){
		//only runs the first time loop is called (otherwise there would be no title)
		selected_menu = 0;
		display_title(titles[selected_menu]);
	}
	//====== display corresponding menu ======
	switch (selected_menu){
	case 0:
		display_national_rail_departures("CFB");
		break;
	case 1:
		display_national_rail_departures("CTF");
		break;
	case 2:
		display_national_rail_departures("HPA");
		break;
	case 3:
		//ravensbourne park crescent and bourneville road (same stop in opposite directions)
		const char *stops[] = {"490011434Z","490000365Z"};
		size_t stop_count = sizeof(stops)/sizeof(const char *);
		display_tfl_arrivals(stops,stop_count);
		break;
	}
	//====== wait for user input ======
	const time_t time_between_updates_ms = 60000;
	const time_t wait_time = 50; //user input poll interval (ms)
	time_t time_waited_ms = 0;
	int title_rendered = 0;
	while (time_waited_ms < time_between_updates_ms){
		delay(wait_time);
		time_waited_ms += wait_time;
		uint16_t x[2], y[2];
		if (inkplate.tsGetData(x,y) > 0){
			selected_menu = (selected_menu+1) % 4;
			display_title(titles[selected_menu]);
			//give user grace time to keep navigating to a new menu
			time_waited_ms = time_between_updates_ms - 500; //0.5s
			title_rendered = 1;
		}
	}
	//====== render the title ======
	Serial.println("displaying title...");
	//if user has scrolled through menus no need to redisplay the title
	if (!title_rendered) display_title(titles[selected_menu]);
}

void display_title(const char *title){
	//====== display header ======
	inkplate.clearDisplay();
	inkplate.setTextColor(BLACK);
	inkplate.setCursor(50,50);
	inkplate.setTextSize(4);
	inkplate.print(title);
	inkplate.fillRect(50,90,500,10,BLACK);
	inkplate.display();
}

int display_national_rail_departures(const char *crs_code){
	//====== fetch train times ======
	Serial.println("requesting departures from "+String(crs_code)+"...");
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
			//====== update screen ======
			inkplate.display();
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

int display_tfl_arrivals(const char **stop_ids, size_t stop_count){
	//====== for each stop point fetch the stops ======
	HTTPClient http;
	int display_line = 0;
	for (int stop = 0; stop < stop_count; stop++){
		const char *stop_id = stop_ids[stop];
		//====== fetch from api ======
		Serial.println("requesting tfl arrivals from "+String(stop_id)+"...");
		if (http.begin("https://api.tfl.gov.uk/StopPoint/"+String(stop_id)+"/Arrivals")){
			int http_response_code = http.GET();
			if (http_response_code == 200){
				//====== deserialise =====
				String response = http.getString();
				//Serial.println(response);
				JsonDocument arrivals;
				deserializeJson(arrivals, response);
				//====== display each train time ======
				inkplate.setTextSize(3);
				if (arrivals.isNull()){
					Serial.println("no services provided");
					continue;
				}
				Serial.println("arrivals fetched successfully");
				for (JsonVariant arrival : arrivals.as<JsonArray>()){
					struct tm expected_arrival_tm = {0};
					strptime(arrival["expectedArrival"] | "","%FT%TZ",& expected_arrival_tm);
					char expected_arrival[25] = "";
					strftime(expected_arrival,sizeof(expected_arrival)/sizeof(char),"%H:%M",&expected_arrival_tm);
					String destination = arrival["destinationName"];
					inkplate.setCursor(40,130+(35*display_line));
					inkplate.print(destination);
					inkplate.setCursor(475,130+(35*display_line));
					inkplate.print(expected_arrival);
					display_line++;
				}
				Serial.println("departures displayed");
			}
		}else {
			Serial.println("could not connect");
		}
	}
	//====== update screen ======
	inkplate.display();
	return 0;
}

int wifi_connect(){
	WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
	while (WiFi.status() != WL_CONNECTED){
	}
	return 0;
}
