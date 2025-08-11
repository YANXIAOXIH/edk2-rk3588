/** @file
*
*  Copyright (c) 2021, Rockchip Limited. All rights reserved.
*  Copyright (c) 2023-2024, Mario Bălănică <mariobalanica02@gmail.com>
*  Copyright (c) 2024, Your Name <your.email@example.com>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/GpioLib.h>
#include <Library/RK806.h>
#include <Library/Rk3588Pcie.h>
#include <Library/PWMLib.h>
#include <Soc.h>
#include <VarStoreData.h>

static struct regulator_init_data  rk806_init_data[] = {
  /* Master PMIC */
  RK8XX_VOLTAGE_INIT (MASTER_BUCK1,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK3,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK4,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK5,  850000),
  // RK8XX_VOLTAGE_INIT(MASTER_BUCK6, 750000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK7,  2000000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK8,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_BUCK10, 1800000),

  RK8XX_VOLTAGE_INIT (MASTER_NLDO1,  750000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO2,  850000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO3,  837500),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO4,  850000),
  RK8XX_VOLTAGE_INIT (MASTER_NLDO5,  750000),

  RK8XX_VOLTAGE_INIT (MASTER_PLDO1,  1800000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO2,  1800000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO3,  1200000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO4,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO5,  3300000),
  RK8XX_VOLTAGE_INIT (MASTER_PLDO6,  1800000),

  /* No dual PMICs on this platform */
};

VOID
EFIAPI
SdmmcIoMux (
  VOID
  )
{
  /* sdmmc0 iomux (microSD socket) */
  BUS_IOC->GPIO4D_IOMUX_SEL_L  = (0xFFFFUL << 16) | (0x1111); // SDMMC_D0,SDMMC_D1,SDMMC_D2,SDMMC_D3
  BUS_IOC->GPIO4D_IOMUX_SEL_H  = (0x00FFUL << 16) | (0x0011); // SDMMC_CLK,SDMMC_CMD
  PMU1_IOC->GPIO0A_IOMUX_SEL_H = (0x000FUL << 16) | (0x0001); // SDMMC_DET
}

VOID
EFIAPI
SdhciEmmcIoMux (
  VOID
  )
{
  /* sdmmc0 iomux */
  BUS_IOC->GPIO2A_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x1111); // EMMC_CMD,EMMC_CLKOUT,EMMC_DATASTROBE,EMMC_RSTN
  BUS_IOC->GPIO2D_IOMUX_SEL_L = (0xFFFFUL << 16) | (0x1111); // EMMC_D0,EMMC_D1,EMMC_D2,EMMC_D3
  BUS_IOC->GPIO2D_IOMUX_SEL_H = (0xFFFFUL << 16) | (0x1111); // EMMC_D4,EMMC_D5,EMMC_D6,EMMC_D7
}

#define NS_CRU_BASE       0xFD7C0000
#define CRU_CLKSEL_CON59  0x03EC

VOID
EFIAPI
Rk806SpiIomux (
  VOID
  )
{
  /* io mux */
  // BUS_IOC->GPIO1A_IOMUX_SEL_H = (0xFFFFUL << 16) | 0x8888;
  // BUS_IOC->GPIO1B_IOMUX_SEL_L = (0x000FUL << 16) | 0x0008;
  PMU1_IOC->GPIO0A_IOMUX_SEL_H = (0x0FF0UL << 16) | 0x0110;
  PMU1_IOC->GPIO0B_IOMUX_SEL_L = (0xF0FFUL << 16) | 0x1011;
  MmioWrite32 (NS_CRU_BASE + CRU_CLKSEL_CON59, (0x00C0UL << 16) | 0x0080);
}

VOID
EFIAPI
Rk806Configure (
  VOID
  )
{
  UINTN  RegCfgIndex;

  RK806Init ();

  RK806PinSetFunction (MASTER, 1, 2); // rk806_dvs1_pwrdn

  for (RegCfgIndex = 0; RegCfgIndex < ARRAY_SIZE (rk806_init_data); RegCfgIndex++) {
    RK806RegulatorInit (rk806_init_data[RegCfgIndex]);
  }
}

VOID
EFIAPI
SetCPULittleVoltage (
  IN UINT32  Microvolts
  )
{
  struct regulator_init_data  Rk806CpuLittleSupply =
    RK8XX_VOLTAGE_INIT (MASTER_BUCK2, Microvolts);

  RK806RegulatorInit (Rk806CpuLittleSupply);
}

VOID
EFIAPI
NorFspiIomux (
  VOID
  )
{
}

VOID
EFIAPI
NorFspiEnableClock (
  UINT32  *CruBase
  )
{
}

VOID
EFIAPI
GmacIomux (
  IN UINT32  Id
  )
{
  /* No GMAC here */
}

VOID
EFIAPI
GmacIoPhyReset (
  UINT32   Id,
  BOOLEAN  Enable
  )
{
  switch (Id) {
    case 0:
      // gmac0 reset: DTS中为 &gpio4 RK_PC6 GPIO_ACTIVE_LOW
      GpioPinWrite (4, GPIO_PIN_PC6, !Enable);
      break;
    case 1:
      // gmac1 reset: DTS中为 &gpio3 RK_PB7 GPIO_ACTIVE_LOW
      GpioPinWrite (3, GPIO_PIN_PB7, !Enable);
      break;
    default:
      break;
  }
}

// 保留: I2C IO配置。您的DTS使用的I2C引脚与OPi5+的配置基本一致，可以直接沿用。
VOID
EFIAPI
I2cIomux (
  UINT32  id
  )
{
  switch (id) {
    case 0:
      GpioPinSetFunction (0, GPIO_PIN_PD1, 3); // i2c0_scl_m2
      GpioPinSetFunction (0, GPIO_PIN_PD2, 3); // i2c0_sda_m2
      break;
    case 1:
      GpioPinSetFunction (0, GPIO_PIN_PD4, 9); // i2c1_scl_m2
      GpioPinSetFunction (0, GPIO_PIN_PD5, 9); // i2c1_sda_m2
      break;
    case 2:
      break;
    case 3:
      GpioPinSetFunction (1, GPIO_PIN_PC1, 9); // i2c3_scl_m0
      GpioPinSetFunction (1, GPIO_PIN_PC0, 9); // i2c3_sda_m0
      break;
    case 4:
      GpioPinSetFunction (4, GPIO_PIN_PC4, 9); // i2c4_scl_m3
      GpioPinSetFunction (4, GPIO_PIN_PC5, 9); // i2c4_sda_m3
      break;
    case 5:
      GpioPinSetFunction (2, GPIO_PIN_PA0, 9); // i2c5_scl_m2
      GpioPinSetFunction (2, GPIO_PIN_PA1, 9); // i2c5_sda_m2
      break;
    case 6:
      GpioPinSetFunction (0, GPIO_PIN_PD0, 9); // i2c6_scl_m0
      GpioPinSetFunction (0, GPIO_PIN_PC7, 9); // i2c6_sda_m0
      break;
    case 7:
      GpioPinSetFunction (1, GPIO_PIN_PD0, 9); // i2c7_scl_m0
      GpioPinSetFunction (1, GPIO_PIN_PD1, 9); // i2c7_sda_m0
      break;
    default:
      break;
  }
}

VOID
EFIAPI
UsbPortPowerEnable (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "UsbPortPowerEnable called for LemonPi\n"));

  /* vcc5v0_host_en */
  GpioPinWrite (2, GPIO_PIN_PB5, TRUE);
  GpioPinSetDirection (2, GPIO_PIN_PB5, GPIO_PIN_OUTPUT);

  /* typec5v_pwren */
  GpioPinWrite (4, GPIO_PIN_PA3, TRUE);
  GpioPinSetDirection (4, GPIO_PIN_PA3, GPIO_PIN_OUTPUT);
}

VOID
EFIAPI
Usb2PhyResume (
  VOID
  )
{
  MmioWrite32 (0xfd5d0008, 0x20000000);
  MmioWrite32 (0xfd5d4008, 0x20000000);
  MmioWrite32 (0xfd5d8008, 0x20000000);
  MmioWrite32 (0xfd5dc008, 0x20000000);
  MmioWrite32 (0xfd7f0a10, 0x07000700);
  MmioWrite32 (0xfd7f0a10, 0x07000000);
}

VOID
EFIAPI
PcieIoInit (
  UINT32  Segment
  )
{
  /* Set reset and power IO to gpio output mode */
  switch (Segment) {
    case PCIE_SEGMENT_PCIE30X4: // M.2 M Key
      /* reset */
      GpioPinSetDirection (1, GPIO_PIN_PB2, GPIO_PIN_OUTPUT);
      break;
    case PCIE_SEGMENT_PCIE20L0: // M.2 A+E Key
      /* reset */
      GpioPinSetDirection (4, GPIO_PIN_PA5, GPIO_PIN_OUTPUT);
      break;
    default:
      break;
  }
}

VOID
EFIAPI
PciePowerEn (
  UINT32   Segment,
  BOOLEAN  Enable
  )
{
  
}

VOID
EFIAPI
PciePeReset (
  UINT32   Segment,
  BOOLEAN  Enable
  )
{
  switch (Segment) {
    case PCIE_SEGMENT_PCIE30X4:
      GpioPinWrite (1, GPIO_PIN_PB2, !Enable);
      break;
    case PCIE_SEGMENT_PCIE20L0:
      GpioPinWrite (4, GPIO_PIN_PA5, !Enable);
      break;
    default:
      break;
  }
}

VOID
EFIAPI
HdmiTxIomux (
  IN UINT32  Id
  )
{
  switch (Id) {
    case 1:
      GpioPinWrite(1, GPIO_PIN_PC6, TRUE);
      GpioPinSetDirection(1, GPIO_PIN_PC6, GPIO_PIN_OUTPUT);
      GpioPinSetFunction (3, GPIO_PIN_PC4, 5);
      GpioPinSetPull (3, GPIO_PIN_PC4, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (1, GPIO_PIN_PA6, 5);
      GpioPinSetPull (1, GPIO_PIN_PA6, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (3, GPIO_PIN_PC6, 5);
      GpioPinSetPull (3, GPIO_PIN_PC6, GPIO_PIN_PULL_NONE);
      GpioPinSetFunction (3, GPIO_PIN_PC5, 5);
      GpioPinSetPull (3, GPIO_PIN_PC5, GPIO_PIN_PULL_NONE);
      break;
  }
}

PWM_DATA  pwm_data = {
  .ControllerID = PWM_CONTROLLER1,
  .ChannelID    = PWM_CHANNEL0,
  .PeriodNs     = 20000,
  .DutyNs       = 20000,
  .Polarity     = FALSE,
}; // PWM0_CH3

VOID
EFIAPI
PwmFanIoSetup (
  VOID
  )
{
  GpioPinSetFunction (1, GPIO_PIN_PC4, 2); // PWM3_IR_M1
  RkPwmSetConfig (&pwm_data);
  RkPwmEnable (&pwm_data);
}

VOID
EFIAPI
PwmFanSetSpeed (
  IN UINT32  Percentage
  )
{
  pwm_data.DutyNs = pwm_data.PeriodNs * Percentage / 100;
  RkPwmSetConfig (&pwm_data);
}

VOID
EFIAPI
PlatformInitLeds (
  VOID
  )
{
  /* Status indicator */
  GpioPinWrite (0, GPIO_PIN_PD3, TRUE);
  GpioPinSetDirection (0, GPIO_PIN_PD3, GPIO_PIN_OUTPUT);
}

VOID
EFIAPI
PlatformSetStatusLed (
  IN BOOLEAN  Enable
  )
{
  GpioPinWrite (0, GPIO_PIN_PD3, !Enable);
}

VOID
EFIAPI
PlatformWiFiEnable (
  IN BOOLEAN  Enable
  )
{
  // WiFi - enable
  GpioPinWrite (0, GPIO_PIN_PC6, !Enable);
  GpioPinSetDirection (0, GPIO_PIN_PC6, GPIO_PIN_OUTPUT);
}

CONST EFI_GUID *
EFIAPI
PlatformGetDtbFileGuid (
  IN UINT32  CompatMode
  )
{
  STATIC CONST EFI_GUID  VendorDtbFileGuid = {
    // DeviceTree/Vendor.inf
    0xd58b4028, 0x43d8, 0x4e97, { 0x87, 0xd4, 0x4e, 0x37, 0x16, 0x13, 0x65, 0x80 }
  };
  STATIC CONST EFI_GUID  MainlineDtbFileGuid = {
    // DeviceTree/Mainline.inf
    0x84492e97, 0xa10f, 0x49a7, { 0x85, 0xe9, 0x02, 0x5d, 0x19, 0x66, 0xb3, 0x43 }
  };

  switch (CompatMode) {
    case FDT_COMPAT_MODE_VENDOR:
      return &VendorDtbFileGuid;
    case FDT_COMPAT_MODE_MAINLINE:
      return &MainlineDtbFileGuid;
  }

  return NULL;
}

VOID
EFIAPI
PlatformEarlyInit (
  VOID
  )
{
  // Configure various things specific to this platform
  PlatformWiFiEnable (TRUE);

  GpioPinSetFunction (1, GPIO_PIN_PD3, 0); // jdet
}
