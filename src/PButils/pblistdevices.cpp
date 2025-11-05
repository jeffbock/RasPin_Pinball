#include "wiringPi.h"
#include "wiringPiI2C.h"
#include <iostream>
#include <iomanip>
#include <vector>

// I2C addresses for different device types

// MAX9744 Amplifier addresses (0x4B-0x4D, 3 possible addresses)
#define AMP_BASE_ADDRESS 0x4B
#define AMP_ADDRESS_COUNT 3

// TLC59116 LED Driver addresses (0x60-0x6F, 16 possible addresses)
#define LED_EXPANDER_BASE_ADDRESS 0x60
#define LED_EXPANDER_ADDRESS_COUNT 16

// TCA9555 IO Expander addresses (0x20-0x27, 8 possible addresses)
#define IO_EXPANDER_BASE_ADDRESS 0x20
#define IO_EXPANDER_ADDRESS_COUNT 8

bool checkDevice(int address, const char* deviceName, bool useRegisterRead) {
  int fd = wiringPiI2CSetup(address);
  
  std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') 
            << address << " - " << deviceName << ": ";
  
  if (fd <= 0) {
    std::cout << "Empty" << std::endl;
    return false;
  }
  
  // Try to communicate with the device to verify it's actually responding
  int result = -1;
  if (useRegisterRead) {
    // For devices with registers (IO Expander, LED Driver), try reading a register
    // Use register 0 as it's commonly available
    result = wiringPiI2CReadReg8(fd, 0x00);
  } else {
    // For simple devices (amplifier), try a simple read
    result = wiringPiI2CRead(fd);
  }
  
  if (result >= 0) {
    std::cout << "Active" << std::endl;
    return true;
  } else {
    std::cout << "Empty (no response)" << std::endl;
    return false;
  }
}

int main(void)
{
  // Initialize wiringPi using BCM numbering
  wiringPiSetupPinType(WPI_PIN_BCM);

  std::cout << "Scanning I2C bus for devices..." << std::endl;
  std::cout << "================================" << std::endl;
  std::cout << std::endl;

  // Vectors to store found device addresses
  std::vector<int> foundAmplifiers;
  std::vector<int> foundLEDExpanders;
  std::vector<int> foundIOExpanders;

  // Check amplifier addresses (MAX9744)
  std::cout << "Amplifier (MAX9744) addresses:" << std::endl;
  for (int i = 0; i < AMP_ADDRESS_COUNT; i++) {
    if (checkDevice(AMP_BASE_ADDRESS + i, "Amplifier", false)) {
      foundAmplifiers.push_back(AMP_BASE_ADDRESS + i);
    }
  }
  std::cout << std::endl;

  // Check LED expander addresses (TLC59116)
  std::cout << "LED Expander (TLC59116) addresses:" << std::endl;
  for (int i = 0; i < LED_EXPANDER_ADDRESS_COUNT; i++) {
    if (checkDevice(LED_EXPANDER_BASE_ADDRESS + i, "LED Expander", true)) {
      foundLEDExpanders.push_back(LED_EXPANDER_BASE_ADDRESS + i);
    }
  }
  std::cout << std::endl;

  // Check IO expander addresses (TCA9555)
  std::cout << "IO Expander (TCA9555) addresses:" << std::endl;
  for (int i = 0; i < IO_EXPANDER_ADDRESS_COUNT; i++) {
    if (checkDevice(IO_EXPANDER_BASE_ADDRESS + i, "IO Expander", true)) {
      foundIOExpanders.push_back(IO_EXPANDER_BASE_ADDRESS + i);
    }
  }
  std::cout << std::endl;

  // Print summary
  std::cout << "Summary:" << std::endl;
  std::cout << "--------" << std::endl;
  
  // Amplifiers summary
  std::cout << "Amplifiers: " << std::dec << foundAmplifiers.size();
  if (!foundAmplifiers.empty()) {
    std::cout << " (";
    for (size_t i = 0; i < foundAmplifiers.size(); i++) {
      if (i > 0) std::cout << ", ";
      std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << foundAmplifiers[i];
    }
    std::cout << ")";
  }
  std::cout << std::endl;
  
  // LED Expanders summary
  std::cout << "LED Expanders: " << std::dec << foundLEDExpanders.size();
  if (!foundLEDExpanders.empty()) {
    std::cout << " (";
    for (size_t i = 0; i < foundLEDExpanders.size(); i++) {
      if (i > 0) std::cout << ", ";
      std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << foundLEDExpanders[i];
    }
    std::cout << ")";
  }
  std::cout << std::endl;
  
  // IO Expanders summary
  std::cout << "IO Expanders: " << std::dec << foundIOExpanders.size();
  if (!foundIOExpanders.empty()) {
    std::cout << " (";
    for (size_t i = 0; i < foundIOExpanders.size(); i++) {
      if (i > 0) std::cout << ", ";
      std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << foundIOExpanders[i];
    }
    std::cout << ")";
  }
  std::cout << std::endl;

  return 0;
}
