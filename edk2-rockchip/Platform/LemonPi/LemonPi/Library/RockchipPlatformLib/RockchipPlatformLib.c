/** @file
*
*  Copyright (c) 2021, Rockchip Limited. All rights reserved.
*  Copyright (c) 2023-2024, Mario Bălănică <mariobalanica02@gmail.com>
*  Copyright (c) 2024, LemonPi Project Authors.
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

  // The kernel device tree configures the RK806 power control pins as
  // "null" (pin_fun0): the big CPU cores are supplied by the RK8602 and
  // RK8603 regulators on I2C0, not by the RK806 DVS outputs.
  RK806PinSetFunction (MASTER, 1, 0); // gpio_pwrctrl1: pin_fun0 (null)
  RK806PinSetFunction (MASTER, 2, 0); // gpio_pwrctrl2: pin_fun0 (null)
  RK806PinSetFunction (MASTER, 3, 0); // gpio_pwrctrl3: pin_fun0 (null)

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
  UINTN  BaseAddr = (UINTN)CruBase;

  /* Enable the FSPI clocks, same as the other Rockchip platforms do.
   * The LemonPi carrier has no SPI NOR, but NorFlashDxe calls this
   * helper unconditionally. */
  MmioWrite32 (BaseAddr + 0x087C, 0x0E000000);
}

VOID
EFIAPI
GmacIomux (
  IN UINT32  Id
  )
{
  /* RGMII pin multiplexing. All the RGMII signals use function 1.
   *
   * Sources:
   *   gmac0: rockchip/rk3588-extra-pinctrl.dtsi (gmac0_miim,
   *           gmac0_tx_bus2, gmac0_rx_bus2, gmac0_rgmii_clk,
   *           gmac0_rgmii_bus)
   *   gmac1: rockchip/rk3588-base-pinctrl.dtsi (gmac1_miim,
   *           gmac1_tx_bus2, gmac1_rx_bus2, gmac1_rgmii_clk,
   *           gmac1_rgmii_bus)
   *
   * The pin selection matches the kernel device tree
   * (rk3588-lemonpi.dts, &gmac0/&gmac1). The MCLKINOUT pins are not
   * muxed here on purpose: the kernel DT does not mux them either, the
   * PHY runs off its own crystal.
   */
  switch (Id) {
    case 0:
      /* gmac0 */
      GpioPinSetFunction (4, GPIO_PIN_PC4, 1); // MDC
      GpioPinSetFunction (4, GPIO_PIN_PC5, 1); // MDIO
      GpioPinSetFunction (4, GPIO_PIN_PC2, 1); // RXD_CRS
      GpioPinSetFunction (2, GPIO_PIN_PC1, 1); // RXD0
      GpioPinSetFunction (2, GPIO_PIN_PC2, 1); // RXD1
      GpioPinSetFunction (2, GPIO_PIN_PB6, 1); // TXD0
      GpioPinSetFunction (2, GPIO_PIN_PB7, 1); // TXD1
      GpioPinSetFunction (2, GPIO_PIN_PC0, 1); // TX_EN
      GpioPinSetFunction (2, GPIO_PIN_PB0, 1); // RX_CLK
      GpioPinSetFunction (2, GPIO_PIN_PB3, 1); // TX_CLK
      GpioPinSetFunction (2, GPIO_PIN_PA6, 1); // RXD2
      GpioPinSetFunction (2, GPIO_PIN_PA7, 1); // RXD3
      GpioPinSetFunction (2, GPIO_PIN_PB1, 1); // TXD2
      GpioPinSetFunction (2, GPIO_PIN_PB2, 1); // TXD3
      break;
    case 1:
      /* gmac1 */
      GpioPinSetFunction (3, GPIO_PIN_PC2, 1); // MDC
      GpioPinSetFunction (3, GPIO_PIN_PC3, 1); // MDIO
      GpioPinSetFunction (3, GPIO_PIN_PB1, 1); // RXD_CRS
      GpioPinSetFunction (3, GPIO_PIN_PA7, 1); // RXD0
      GpioPinSetFunction (3, GPIO_PIN_PB0, 1); // RXD1
      GpioPinSetFunction (3, GPIO_PIN_PB3, 1); // TXD0
      GpioPinSetFunction (3, GPIO_PIN_PB4, 1); // TXD1
      GpioPinSetFunction (3, GPIO_PIN_PB5, 1); // TX_EN
      GpioPinSetFunction (3, GPIO_PIN_PA5, 1); // RX_CLK
      GpioPinSetFunction (3, GPIO_PIN_PA4, 1); // TX_CLK
      GpioPinSetFunction (3, GPIO_PIN_PA2, 1); // RXD2
      GpioPinSetFunction (3, GPIO_PIN_PA3, 1); // RXD3
      GpioPinSetFunction (3, GPIO_PIN_PA0, 1); // TXD2
      GpioPinSetFunction (3, GPIO_PIN_PA1, 1); // TXD3
      break;
    default:
      break;
  }
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
      // gmac0 PHY reset: DTS 中为 &gpio4 RK_PC6 GPIO_ACTIVE_LOW
      // GpioPinWrite() 只写数据寄存器，方向必须单独设置，
      // 否则复位脚仍为输入，PHY 永远不会被复位。
      GpioPinSetDirection (4, GPIO_PIN_PC6, GPIO_PIN_OUTPUT);
      GpioPinWrite (4, GPIO_PIN_PC6, !Enable);
      break;
    case 1:
      // gmac1 PHY reset: DTS 中为 &gpio3 RK_PB7 GPIO_ACTIVE_LOW
      GpioPinSetDirection (3, GPIO_PIN_PB7, GPIO_PIN_OUTPUT);
      GpioPinWrite (3, GPIO_PIN_PB7, !Enable);
      break;
    default:
      break;
  }
}

// I2C IO configuration. The pin selection follows the kernel device tree
// (rk3588-lemonpi.dts) and rockchip/rk3588-base-pinctrl.dtsi.
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
      // i2c4m3 is on GPIO1_A3/A2. The previous code used GPIO4_C4/C5,
      // which are the gmac0 MDC/MDIO pins and would break Ethernet.
      GpioPinSetFunction (1, GPIO_PIN_PA3, 9); // i2c4_scl_m3
      GpioPinSetFunction (1, GPIO_PIN_PA2, 9); // i2c4_sda_m3
      break;
    case 5:
      // i2c5m2 is on GPIO4_A6/A7 (PCA9535 expander on the carrier)
      GpioPinSetFunction (4, GPIO_PIN_PA6, 9); // i2c5_scl_m2
      GpioPinSetFunction (4, GPIO_PIN_PA7, 9); // i2c5_sda_m2
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

  /* TYPE_C_PWR_EN (vbus5v0_typec, GPIO2_B5, active high) switches the
   * 5V rail of the Type-C port. The USB host ports are supplied by
   * vcc5v0_usb, which is always on and has no GPIO enable. */
  GpioPinWrite (2, GPIO_PIN_PB5, TRUE);
  GpioPinSetDirection (2, GPIO_PIN_PB5, GPIO_PIN_OUTPUT);
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
      /* HDMI1: CEC (GPIO3_C4), HPD (GPIO1_A6), SCL (GPIO3_C6),
       * SDA (GPIO3_C5), all on function 5. */
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

/* SYS_FAN is driven by PWM4_M0, i.e. GPIO0_C5 with function 11 (see the
 * kernel device tree &pwm4 / &pwm4m0_pins and schematic sheet 13).
 * PWM_CONTROLLER1 + PWM_CHANNEL0 is pwm4 channel 0 (0xfebd0000) and the
 * 50000ns period matches the "50000" used by &pwm4 in the device tree. */
PWM_DATA  pwm_data = {
  .ControllerID = PWM_CONTROLLER1,
  .ChannelID    = PWM_CHANNEL0,
  .PeriodNs     = 50000,
  .DutyNs       = 50000,
  .Polarity     = FALSE,
}; // PWM4_M0

VOID
EFIAPI
PwmFanIoSetup (
  VOID
  )
{
  GpioPinSetFunction (0, GPIO_PIN_PC5, 11); // pwm4_m0 (SYS_FAN)
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
  // M.2 E-Key disable line (GPIO0_PC6). The device tree uses it as
  // "disable-gpios" with GPIO_ACTIVE_HIGH, so a low level enables the
  // module and a high level disables it.
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
}
