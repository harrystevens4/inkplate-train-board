
# About

![Catford Bridge Departures rendered on Inkplate 4TEMPERA](/images/catford-bridge-departures.jpg)

This is a project to create a train board like the ones at stations. It uses the rail data marketplace live departure board API to fetch the train times from national rail stations [API link](https://raildata.org.uk/dashboard/dataProduct/P-d81d6eaf-8060-4467-a339-1c833e50cbbe/overview).

# How to use it yourself

Requires the `ArduinoJson` library by `Benoit` and the `InkplateLibrary` library. You will need to create a header file in the main folder called `wifi_password.h` that defines `WIFI_SSID` and `WIFI_PASSWORD` for your network. You will also need to create a header file called `keys.h` which defines `LDBWS_DEPARTURES_KEY` which should be your consumer key found [here](https://raildata.org.uk/dashboard/dataProduct/P-d81d6eaf-8060-4467-a339-1c833e50cbbe/specification). You may need to rename this folder to `inkplate` for Arduino IDE to recognise it.

The current station being displayed can be changed by pressing the wake button on the side. It should cycle through each of the stations. The station departures are updated every minute. I have it go into light sleep between updates to conserve battery, so when it is woken by the wake button it may take a second before it updates, as it has to reinitialise WiFi.

# Modification

Feel free to change it yourself to suit your needs. My c++ is pretty grim so if you can make it better I'm happy to accept pull requests. If you want to use different stations, you can find their CRS code [here](https://www.nationalrail.co.uk/stations/) (its the 3 letter code next to the station's name).
