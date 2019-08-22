# TLS3001 with ESP32 SPI driver

This repository contains code to communicate with the TLS3001 led-drivers. It's still under development. 

## Toolchain and ESP-IDF versions:
*  Currently builds under ESP-IDF version: v4.0-dev-1191-g138c941fa (Should work with bleeding edge as well as stable v.3.2)  
*  Toolchain: xtensa esp gcc version 5.2.0

## Examples and how-to
The library sends data over SPI to the TLS3001 drivers in a task called *TLS3001_task()*. See the ``send_to_TLS3001_task_template.c`` for a template on how to do this.  

The code also contains a command-line interface. In order to build with this, the following flag in menuconfig must be set: ``CONFIG_FREERTOS_USE_TRACE_FACILITY=y`` 
