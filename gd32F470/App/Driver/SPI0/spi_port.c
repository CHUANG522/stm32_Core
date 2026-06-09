#include "spi_port.h"

void ad3344_spi_init(void)
{ 
    /*!< SPI pins configuration *************************************************/  
    /* SPI0 GPIO config: CS/PE2, SCK/PA5, MISO/PA6, MOSI/PA7 */
    rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOE);
    rcu_periph_clock_enable(RCU_SPI0);
	

    /* 配置 PA4 为推挽输出 (CS 片选) */
    gpio_mode_set(GPIOE, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_2);
    gpio_output_options_set(GPIOE, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);

    /* 配置 PA5/PA6/PA7 为 SPI 复用功能 (AF5) */
    gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_5);  // SCK
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_6);  // MISO
    gpio_af_set(GPIOA, GPIO_AF_5, GPIO_PIN_7);  // MOSI
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);

    spi_parameter_struct spi_init_struct;
    spi_i2s_deinit(SPI0);
    spi_struct_para_init(&spi_init_struct);

    /* SPI0 parameter config (与原代码时序完全一致) */
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_16BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;  // CPOL=0, CPHA=1
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_32;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(SPI0, &spi_init_struct);

    spi_enable(SPI0);
    
    SPI_SET_CS();    // 拉高片选
}

uint16_t ad3344_spi_txrx16bit(uint16_t tx_byte)
{
    /*!< Loop while DR register in not empty */
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_TBE));
    
    /*!< Send byte through the SPI0 peripheral */
    spi_i2s_data_transmit(SPI0, tx_byte);
    
    /*!< Wait to receive a byte */
    while(RESET == spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    
    /*!< Return the byte read from the SPI bus */
    return spi_i2s_data_receive(SPI0);
}
