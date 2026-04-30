
# EduDemoS Sun-Tracker

This "Sun-Tracker" is our most advanced demonstrator. Similar to a real sunflower, it turns its flower towards the sun to collect as much light as possible. Like the Modular Weather Station, it can send data to the Internet via MQTT. For your convenience there are two sets of instructions. One includes step-by-step instructions for building the complete Sun-Tracker. The other is an abbreviated version suitable for a four-hour workshop and assumes that all soldering and other preparatory work has already been completed.

This repository contains all necessary files and is structured as follows:

- [doc](doc) contains documentation such as
  - assembly manual
  - bill of material
- [code](code) contains program code to run the model
  - [Sun-Tracker preparation firmware](code/Sun-Tracker_Preparation)
  - [Sun-Tracker offline](code/Sun-Tracker)
  - [Sun-Tracker 🌐 IoT ready](code/sun_tracker_iot)
- [dashboards](dashboards) contains example dashboards for ThingsBoard (see [description below](#using-the-example-dashboards)).
- [model](model) contains the printable 3D model

## Using the example dashboards

The example dashboards are tailored for the [IoT variant of the control software](code/sun_tracker_iot/)
To use the example dashboards:

1. Import the JSON file as ThingsBoard dashboard
2. Enter edit mode
3. For each widget adjust:
   - if the target device field is empty: Enter the appropriate target device.
   - if the target device field is not empty, the dashboard features an [entity alias](https://thingsboard.io/docs/user-guide/ui/aliases/#single-entity) to reference the target device. In this case [edit the alias](https://thingsboard.io/docs/user-guide/ui/aliases/#creating-alias) to have it pointing towards the appropriate target device.
4. Save the dashboard

## Licensing

Please note: different licenses apply depending on the type of content.

All documents and 3d models (files ending with .pdf and .stl) are licensed under [CC-BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/).

All files containing code are licensed under [GNU GPL V3](https://www.gnu.org/licenses/gpl-3.0.txt).

## Disclaimer:

Funded by the European Union. Views and opinions expressed are however those of the author(s) only and do not necessarily reflect those of the European Union or the European Education and Culture Executive Agency (EACEA). Neither the European Union nor EACEA can be held responsible for them.
