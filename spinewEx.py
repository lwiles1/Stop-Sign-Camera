import RPi.GPIO as GPIO
import spidev

GPIO.setmode(GPIO.BCM)  # Sets the GPIO pin labelling mode
SPI_SPEED = 10000  # SPI speed in Hz
spi = spidev.SpiDev()


def setup()
    """Setup GPIO port for CS pin, and the SPI bus"""
    # setup CS
    CS_PIN = 17
    GPIO.setup(CS_PIN, GPIO.OUT)
    GPIO.output(CS_PIN, 1)  # set CS initially to high. CS is pulled low to start a transfer
    # setup SPI
    spi.open(0, 0)  # we are manually toggling CS, so just select channel 0 (ignored)
    spi.max_speed_hz = SPI_SPEED
    spi.mode = 0
def write(data: str):
    """Write data to the device"""
    print(f'-- Writing to device')
    print(f'   sending "{data}"')
    rw_byte = 0x01
    nbytes = len(data)
    command_packet = bytes([rw_byte, nbytes])
    response = _spi_xfer(command_packet)
    data_packet = data.encode('utf-8')
    _spi_xfer(data_packet)
    
def _spi_xfer(to_write: bytes):
    """
    SPI transfer. 
    Toggles the CS pin, and transfers the data in to_write, while reading from the device.
    To read from the device, set to_write to all zeros
    """
    # assert CS
    GPIO.output(CS_PIN, 0)
    
    # do the actual transfer
    response = bytes(spi.xfer2(to_write))
    
    # release CS
    GPIO.output(CS_PIN, 1)
    
    return response
    

if __name__ == '__main__':
    setup()
    write('abcd')
    GPIO.cleanup()