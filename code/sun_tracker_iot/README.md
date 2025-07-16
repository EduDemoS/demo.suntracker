# Solar Demonstrator "Sun-Tracker" ( :globe_with_meridians: IoT-Ready )

This sketch (`.ino`-file) contains a modified version of the original sun_tracker-firmware, augmented with a WiFi/MQTT-based uplink to an MQTT broker.

## Running this application

In order to run this application execute the following steps:

1. Create a copy of the `configuration.default.cpp` file, acting as a template. Name the copy to `configuration.cpp`.
2. Open `configuration.cpp` for editing.
3. Fill in the WiFi’s data with the  information provided by your workshop host:
   - SSID 
   - password 
4. Fill in the MQTT data as assigned by your workshop host:
   - Team ID
   - Username 
   - Password
5. Doublecheck the MQTT configuration with your workshop instructions:
   - Server 
   - Port 

## MQTT concept

This project exposes multiple datapoints that can be used. Inputs and outputs are separated by their path:
- Inputs: `ctrl/...`
- Outputs `data/...`

The available datapoints are:
| **Key**     | **Direction** | **MQTT path**    | **Format**              | **Description**                                                    |
| :---------- | :-----------: | :--------------- | :---------------------- | :----------------------------------------------------------------- |
| _azimuth_   |      Out      | `data/azimuth`   | [Scalar](#scalars)      | Number of ticks, the sunflower is rotated around the Z axis        |
| _elevation_ |      Out      | `data/elevation` | [Scalar](#scalars)      | The sunflower head's angle around the X axis                       |
| _heartbeat_ |      Out      | `data/heartbeat` | [Heartbeat](#heartbeat) | Heartbeat signal sent every second to signal activity              |
| _ldr_       |      Out      | `data/ldr`       | [LDR](#ldr)             | All four LDR values building the foundation for the solar tracking |
| _mirror_    |      Out      | `data/mirror`    | [Plain](#plain-values)  | see [Mirror feature](#mirror-feature)                              |
| _mirror_    |      In       | `ctrl/mirror`    | [Plain](#plain-values)  | see [Mirror feature](#mirror-feature)                              |
| _solar_     |      Out      | `data/solar`     | [Scalar](#scalars)      | Voltage produced on the solar cell                                 |

### Plain values

Plain values do not have any specific format. The values are transferred without any special markup.

### Scalars

For simplified processing on the ThingsBoard end and better forward compatibility, every data point is wrapped within a JSON-object. For scalar values, the object is structured as follows:

```json
{
  "value": <the value>
}
```

Objects of such a structure are automatically interpret by the EduDemoS ThingsBoard instance to scalar key-value-pairs. However, it also allows for further extension of the datapoint without breaking existing data formats by simply adding further keys to the JSON-object in the future.

### Heartbeat

The heartbeat signal is a JSON-object consisting of two keys:
- state - a toggling string-value switching between `on` and `off` automatically.
- mirror - same value as the datapoint `data/mirror`

```json
{
  "state": "off",
  "mirror": false
}
```

### LDR

This JSON-object represents the measurement values for the four LDRs enabling the tracking algorithm. The values transmitted represent the raw values also processed by the software.

While LDRs on the demonstrator are numbered 1..4, the LDRs in the JSON-object are numbered with a zero based index (0..3) as it is common in software engineering.

```json
{
  "ldr0": 18,
  "ldr1": 16,
  "ldr2": 9,
  "ldr3": 0
}
```

### Mirror feature

For demonstrating downlink (i.e. sending through the broker to the device), the software features a mirroring service. This service exposes an input under `ctrl/mirror` and an output through `data/mirror`.

The service follows the following logic:

Whenever a value is set for `ctrl/mirror`, the value is interpret by the following rule:

|  `ctrl/mirror`  | `data/mirror` |
| :-------------: | :-----------: |
|       on        |      off      |
| _anything else_ |      on       |
