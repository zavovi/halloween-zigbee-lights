# Halloween/Christmas Zigbee Lights

Zigbee on/off light switch for Halloween/Christmas lights (battery powered)

## Features
- Set On/Off Lights
- Set Brightness of Lights (PWM using ledc; Working during sleep; Can be disabled in menuconfig)
- Power saving mode (light-sleep)

## Hardware

- [Beetle ESP32-C6](https://wiki.dfrobot.com/SKU_DFR1117_Beetle_ESP32_C6)
- [Gravity MOSFET Power Controller](https://wiki.dfrobot.com/Gravity__MOSFET_Power_Controller_SKU__DFR0457)
    - Connected on **GPIO4** pin

## Build and Flash

Before flashing, it is recommended to erase flash:

```
idf.py -p {port} erase-flash
```

To build and flash the project, use the following command:

```
idf.py -p {port} flash monitor
```