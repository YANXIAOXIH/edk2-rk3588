/** @file
 *
 *  Differentiated System Definition Table (DSDT)
 *
 *  Copyright (c) 2020, Pete Batard <pete@akeo.ie>
 *  Copyright (c) 2018-2020, Andrey Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Copyright (c) 2021, ARM Limited. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include "AcpiTables.h"

#define BOARD_I2S0_TPLG "i2s-jack"

#define BOARD_AUDIO_CODEC_HID "ESSX8388"
#define BOARD_CODEC_I2C "\\_SB.I2C7"
#define BOARD_CODEC_I2C_ADDR 0x11
#define BOARD_CODEC_GPIO "\\_SB.GPI1"
#define BOARD_CODEC_GPIO_PIN GPIO_PIN_PD3

DefinitionBlock ("Dsdt.aml", "DSDT", 2, "RKCP  ", "RK3588  ", 2)
{
  Scope (\_SB_)
  {
    include ("DsdtCommon.asl")

    include ("Cpu.asl")

    include ("Pcie.asl")
    include ("Sata.asl")
    include ("Emmc.asl")
    include ("Sdhc.asl")
    include ("Dma.asl")
    include ("Gmac.asl") // 启用 GMAC 支持
    include ("Gpio.asl")
    include ("I2c.asl")
    include ("Uart.asl")
    include ("Spi.asl") // 启用 SPI 支持

    include ("I2s.asl")

    include ("Usb2Host.asl")
    include ("Usb3Host0.asl")
    include ("Usb3Host1.asl")

    // 定义 I2C7 节点，用于 ES8388 音频编解码器
    Scope (I2C7) {
      Device (CODEC) {
        Name (_HID, "ESSX8388") // 音频编解码器硬件 ID
        Name (_CRS, ResourceTemplate () {
          I2cSerialBusV2 (0x11, ControllerInitiated, 400000,
            AddressingMode7Bit, "\\_SB.I2C7",
            0x00, ResourceConsumer, , Exclusive,
          )
          GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionNone,
            "\\_SB.GPI1", 0, ResourceConsumer, ,
            )
        })
      }
    }

    // 定义 GPIO 节点，用于系统状态 LED
    Device (LED) {
      Name (_HID, "PNP0C02") // 通用设备 ID
      Name (_CRS, ResourceTemplate () {
        GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionNone,
          "\\_SB.GPI0", 0, ResourceConsumer, ,
        )
      })
    }

    // 定义 SATA 控制器
    Device (SATA) {
      Name (_HID, "PNP0A08") // SATA 控制器硬件 ID
      Name (_CRS, ResourceTemplate () {
        Memory32Fixed (ReadWrite, 0xFD5C0000, 0x1000) // 示例地址
        Interrupt (ResourceConsumer, Level, ActiveHigh, Exclusive, ,, ) { 0x1A }
      })
    }

    // 定义 PWM 风扇
    Device (FAN) {
      Name (_HID, "PWMFAN") // PWM 风扇硬件 ID
      Name (_CRS, ResourceTemplate () {
        GpioIo (Exclusive, PullDefault, 0, 0, IoRestrictionNone,
          "\\_SB.GPI4", 0, ResourceConsumer, ,
        )
      })
    }
  }
}