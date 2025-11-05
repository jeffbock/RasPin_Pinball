#include "wiringPi.h"
#include "wiringPiI2C.h"
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <cstring>

// Default I2C address for MAX9744 (AD pin tied to GND)
#define DEFAULT_I2C_ADDRESS 0x4B

int main(int argc, char* argv[])
{
  int i2cAddress = DEFAULT_I2C_ADDRESS;
  int volumeArgIndex = 1;

  // Check for proper usage
  if (argc < 2 || argc > 4) {
    std::cout << "Usage: " << argv[0] << " [--address <addr>] <volume>" << std::endl;
    std::cout << "  --address <addr>: I2C address (0x4B, 0x4C, or 0x4D). Default is 0x4B" << std::endl;
    std::cout << "  volume: 0-63 (0x00-0x3F), where 0 is mute and 63 is max" << std::endl;
    return 1;
  }

  // Parse optional address argument
  if (argc >= 3 && strcmp(argv[1], "--address") == 0) {
    char* endptr;
    errno = 0;
    long addr = strtol(argv[2], &endptr, 16);
    
    // Check for parsing errors
    if (errno != 0 || *endptr != '\0' || endptr == argv[2]) {
      std::cout << "Error: Invalid I2C address format" << std::endl;
      return 1;
    }
    
    // Validate address is one of the valid MAX9744 addresses
    if (addr != 0x4B && addr != 0x4C && addr != 0x4D) {
      std::cout << "Error: I2C address must be 0x4B, 0x4C, or 0x4D" << std::endl;
      return 1;
    }
    
    i2cAddress = (int)addr;
    volumeArgIndex = 3;
  }

  // Parse the volume argument with error handling
  char* endptr;
  errno = 0;
  long volume = strtol(argv[volumeArgIndex], &endptr, 10);
  
  // Check for parsing errors
  if (errno != 0 || *endptr != '\0' || endptr == argv[volumeArgIndex]) {
    std::cout << "Error: Invalid volume value. Please provide a number between 0 and 63" << std::endl;
    return 1;
  }
  
  // Validate volume range (MAX9744 supports 0-63)
  if (volume < 0 || volume > 63) {
    std::cout << "Error: Volume must be between 0 and 63" << std::endl;
    return 1;
  }

  uint8_t volumeValue = (uint8_t)volume;

  // Initialize wiringPi using BCM numbering
  wiringPiSetupPinType(WPI_PIN_BCM);

  // Open up the I2C bus to the MAX9744 Amplifier
  int fd = wiringPiI2CSetup(i2cAddress);

  if (fd <= 0) {
    std::cout << "Error: Failed to open I2C bus to MAX9744 amplifier at address 0x" << std::hex << i2cAddress << std::dec << std::endl;
    return 1;
  }

  // Write the volume value to the amplifier
  int result = wiringPiI2CRawWrite(fd, &volumeValue, 1);
  
  if (result == 1) {
    std::cout << "Successfully set amplifier volume to: " << std::dec << (int)volumeValue << " (0x" << std::hex << (int)volumeValue << std::dec << ") at address 0x" << std::hex << i2cAddress << std::dec << std::endl;
    return 0;
  } else {
    std::cout << "Error: Failed to set amplifier volume" << std::endl;
    return 1;
  }
}
