#ifndef ELITE_SPIDEV
#define ELITE_SPIDEV

#define SPI_STM_SPEED  4000000

typedef struct{
    int fd;                     //! Spi file descriptor
	const char *device;			//! ex: "/dev/spidev1.1";
	uint32_t mode;				//! Spi mode (SPI_LOOP | SPI_CPHA | SPI_CPOL | SPI_LSB_FIRST | SPI_CS_HIGH | SPI_3WIRE | SPI_NO_CS | SPI_READY | SPI_TX_DUAL | SPI_TX_QUAD | SPI_LOOP | SPI_RX_DUAL | SPI_RX_QUAD)
	uint8_t bits;				//! Spi word size in bits (8, 16, 32);
	uint32_t speed;				//! Spi speed in Hz (ex: 4000000)
	uint16_t delay;				//! If nonzero, how long to delay after the last bit transfer before optionally deselecting the device before the next transfer. (in µ sec)
}spi_dev_t;

int elite_spi_init(spi_dev_t *spi_struct, char *spi_device, uint32_t spi_speed, uint32_t spi_mode);
int elite_spi_transfer(spi_dev_t *spi_struct, uint8_t const *tx, uint8_t const *rx, size_t len);
void elite_spi_close(spi_dev_t *spi_struct);

#endif //ELITE_SPIDEV