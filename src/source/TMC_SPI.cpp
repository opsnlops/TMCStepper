
#include "../TMCStepper.h"

using namespace TMCStepper_n;
using namespace TMC_HAL;

int8_t TMC_SPI::chain_length = 0;
uint32_t TMC_SPI::spi_speed = 16000000/8;

TMC_SPI::TMC_SPI(SPIClass &spi, TMC_HAL::PinDef &cs, int8_t link) :
  pinCS(cs),
  TMC_HW_SPI(&spi),
  link_index(link)
  {
    if (link > chain_length)
      chain_length = link;
  }

TMC_SPI::TMC_SPI(SW_SPIClass &spi, TMC_HAL::PinDef &cs, int8_t link) :
  pinCS(cs),
  TMC_SW_SPI(&spi),
  link_index(link)
  {
    if (link > chain_length)
      chain_length = link;
  }

__attribute__((weak))
void TMC_SPI::setSPISpeed(uint32_t speed) {
  spi_speed = speed;
}

void TMC_SPI::transfer(const uint8_t count) {
  char emptyBytes[16] = {0};
  transfer(emptyBytes, count);
}

__attribute__((weak))
uint32_t TMC_SPI::read(uint8_t addressByte) {
  TransferData data;
  OutputPin cs(pinCS);

  beginTransaction();
  cs.write(LOW);
  data.address = addressByte;
  transfer(data.buffer, 5);

  // Shift the written data to the correct driver in chain
  // Default link_index = -1 and no shifting happens
  int8_t i = 1;
  for (; i < link_index; i++) {
    transfer(5);
  }

  cs.write(HIGH);
  cs.write(LOW);

  // Shift data from target link into the last one...
  for (; i < chain_length; i++) {
    transfer(5);
  }

  // ...and once more to MCU
  transfer(data.buffer, 5);

  data.data = __builtin_bswap32(data.data);
  status_response = data.status;

  endTransaction();
  cs.write(HIGH);

  return data.data;
}

__attribute__((weak))
void TMC_SPI::write(uint8_t addressByte, uint32_t config) {
  OutputPin cs(pinCS);
  TransferData data;
  addressByte |= TMC_WRITE;
  data.address = addressByte;
  data.data = __builtin_bswap32(config);

  beginTransaction();
  cs.write(LOW);
  transfer(data.buffer, 5);

  // Shift the written data to the correct driver in chain
  // Default link_index = -1 and no shifting happens
  for (int8_t i = 1; i < chain_length; i++) {
    transfer(5);
  }

  status_response = data.status;

  endTransaction();
  cs.write(HIGH);
}

SW_SPIClass::SW_SPIClass(PinDef mosi, PinDef miso, PinDef sck) :
  mosi_pin(mosi),
  sck_pin(sck),
  miso_pin(miso)
  {}

void SW_SPIClass::init() {
  OutputPin mosi(mosi_pin), sck(sck_pin);
  InputPin miso(miso_pin);

  mosi.setMode();
  sck.setMode();
  miso.setMode();
  sck.write(HIGH);
}

void SW_SPIClass::transfer(char *buf, uint8_t count) {

  OutputPin mosi(mosi_pin), sck(sck_pin);
  InputPin miso(miso_pin);

  auto tx = [&](const uint8_t ulVal){
    uint8_t value = 0;
    sck.reset();

    for (uint8_t i=7 ; i>=1 ; i--) {
      // Write bit
      !!(ulVal & (1 << i)) ? mosi.set() : mosi.reset();
      // Start clock pulse
      sck.set();
      // Read bit
      value |= ( miso.read() ? 1 : 0) << i;
      // Stop clock pulse
      sck.reset();
    }

    !!(ulVal & (1 << 0)) ? mosi.set() : mosi.reset();
    sck.set();
    value |= ( miso.read() ? 1 : 0) << 0;

    return value;
  };

  for (uint8_t i = 0; i<count; i++) {
    *buf = tx(*buf);
    buf++;
  }
}
