
# About

This is a project to create a train board like the ones at stations. It uses the rail data marketplace live departure board API to fetch the train times from national rail stations [API link](https://raildata.org.uk/dashboard/dataProduct/P-d81d6eaf-8060-4467-a339-1c833e50cbbe/overview).

# How to use it yourself

Requires the `ArduinoJson` library by `Benoit` and the `InkplateLibrary` library. You will need to create a header file in the main folder called `wifi_password.h` that defines `WIFI_SSID` and `WIFI_PASSWORD` for your network. You will also need to create a header file called `keys.h` which defines `LDBWS_DEPARTURES_KEY` which should be your consumer key found [here](https://raildata.org.uk/dashboard/dataProduct/P-d81d6eaf-8060-4467-a339-1c833e50cbbe/specification).

The current station being displayed can be changed by pressing the wake button on the side. It should cycle through each of the stations. The station departures are updated every minute/
