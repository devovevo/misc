/******************************************************************************
* Copyright (C) 2023 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/
/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <xparameters.h>
#include "platform.h"
#include "xil_printf.h"
#include "xgpiops.h" // The GPIO driver
#include "sleep.h"   // For usleep

#define EMIO_ENABLE_PIN 54

int main() {
    init_platform();

    XGpioPs Gpio;
    XGpioPs_Config *Config;

    // 1. Initialize the GPIO Driver
    Config = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR);
    XGpioPs_CfgInitialize(&Gpio, Config, Config->BaseAddr);

    // 2. Set the EMIO pin as an Output
    XGpioPs_SetDirectionPin(&Gpio, EMIO_ENABLE_PIN, 1);
    XGpioPs_SetOutputEnablePin(&Gpio, EMIO_ENABLE_PIN, 1);

    print("Going to try and loop over led, turning on and off the blinking!\n\r");

    int led_enable = 1;
    while(1) {
        // 3. Flip the switch to '1'
        // This sends a high signal to your Verilog 'blink_en' port
        xil_printf("Writing %i to Pin 54...\n\r", led_enable);
        XGpioPs_WritePin(&Gpio, EMIO_ENABLE_PIN, led_enable);
        xil_printf("Done. Check LED.\n\r");

        led_enable = led_enable ? 0 : 1;
        sleep(30);
    }

    cleanup_platform();
    return 0;
}