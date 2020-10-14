#pragma once

#include <stdint.h>
#include <plt_hal_uart_base.hpp>
#include <drivers/uart.hpp>
#include <ll_uart.hpp>

#include <cstddef>

extern "C" {
  #include <libopencm3/stm32/usart.h>
  #include <libopencm3/stm32/gpio.h>
  #include <libopencm3/cm3/nvic.h>
  #include <libopencm3/stm32/dma.h>
  #include <libopencm3/stm32/rcc.h>
}

namespace {
  using namespace drivers::uart;
  using namespace platform::ll_drivers::uart;
  using namespace platform::ll_drivers::dma;
  
  template<class T>
  static inline void check_and_call(T* func_ptr, UartDriver* ctx)
  {
    if (func_ptr != nullptr)
    {
      func_ptr(ctx);
    }
  }
}

namespace platform::hal::uart
{ 
  inline void _hal_handle_uart_dma_rx_isr(void* ctx, dma_flags_t flags)
  {
    STM32UartDriver* driver = reinterpret_cast<STM32UartDriver*>(ctx);

    //Transmision completed
    if (flags & DMA_TCIF)
    {
      driver->rx_state = UartState::IDLE;
      check_and_call(driver->rx_completed_cb, driver);
    }
  }

  inline void _hal_handle_uart_dma_tx_isr(void* ctx, dma_flags_t flags)
  {
    STM32UartDriver* driver = reinterpret_cast<STM32UartDriver*>(ctx);

    //Transmision completed
    if (flags & DMA_TCIF)
    {
      driver->tx_state = UartState::IDLE;
      check_and_call(driver->tx_completed_cb, driver);
    }
  }

  inline void _hal_handle_uart_isr([[maybe_unused]] void* ctx, uart_flags_t flags)
  {
    STM32UartDriver* driver = reinterpret_cast<STM32UartDriver*>(ctx);
    
    //Uart is idle
    if (flags & USART_FLAG_IDLE)
    {
      driver->rx_state = UartState::IDLE;
      check_and_call(driver->rx_end_cb, driver);
    }
  }

  inline void init()
  {
    rcc_periph_clock_enable(RCC_USART1);
  }

  template<class TConfig>
  UartDriver* setup()
  {
    using Config = TConfig;

    static_assert(Config::uart_id == USART1);

    //Configure GPIO for UART //TODO: Currently hardcoded for USART1
    platform::ll_drivers::uart::configure_uart_gpio<Config::uart_id>();

    //Configure mode
    constexpr auto mode = get_mode<Config::mode>();
    static_assert(mode != platform::hal::uart::INVALID_MODE);
    usart_set_mode(Config::uart_id, mode);

    //Configure baudrate
    constexpr auto baudrate = get_baudate<Config::baudrate>();
    static_assert(baudrate != platform::hal::uart::INVALID_BAUDRATE);
    usart_set_baudrate(Config::uart_id, baudrate);

    //Configure parity
    constexpr auto parity = get_parity<Config::parity>();
    static_assert(parity != platform::hal::uart::INVALID_PARITY);
    usart_set_parity(Config::uart_id, parity);

    //Configure databits
    constexpr auto databits = get_databits<Config::databits>();
    static_assert(databits != platform::hal::uart::INVALID_DATABITS);
    usart_set_databits(Config::uart_id, databits);
    
    //Configure stopbits
    constexpr auto stopbits = get_stopbits<Config::stopbits>();
    static_assert(stopbits != platform::hal::uart::INVALID_STOPBITS);
    usart_set_stopbits(Config::uart_id, stopbits);

    //Configure flow control
    constexpr auto flow_control = get_flow_control<Config::flow_control>();
    static_assert(flow_control != platform::hal::uart::INVALID_FLOW_CONTROL);
    usart_set_flow_control(Config::uart_id, flow_control);

    //Configure interrupts TODO: Interrupts are enabled when recive is called, function below just enable interrupt in NVIC
    platform::ll_drivers::uart::configure_interrupts<Config::uart_id>();

    //Get driver 
    constexpr STM32UartDriver* driver = platform::ll_drivers::uart::get_driver<Config::uart_id>();
    static_assert(driver != nullptr, "Invalid driver selected. Possibly not yet implemented");
    driver->uart_id = Config::uart_id;
    driver->fwd_isr = _hal_handle_uart_isr;
    driver->fwd_isr_ctx = driver;

    //Configure DMA 
    platform::ll_drivers::uart::configure_uart_dma<Config::uart_id>(driver);
    
    //Fwd function when RX
    driver->rx_dma->fwd_isr = _hal_handle_uart_dma_rx_isr;
    driver->rx_dma->fwd_isr_ctx = driver;

    //Fwd function when TX
    driver->tx_dma->fwd_isr = _hal_handle_uart_dma_tx_isr;
    driver->tx_dma->fwd_isr_ctx = driver;

    //Allow DMA to take control
    usart_enable_rx_dma(driver->uart_id);
    usart_enable_tx_dma(driver->uart_id);
    usart_enable(driver->uart_id);

    return driver;
  }

  inline void receive(drivers::uart::UartDriver* driver, uint8_t* buff, const size_t sz)
  {
    auto stm_driver = reinterpret_cast<STM32UartDriver*>(driver); //TODO: Temp solution. Avoid downcasting

    dma_disable_stream(stm_driver->rx_dma->dma, stm_driver->rx_dma->stream);
    dma_set_memory_address(stm_driver->rx_dma->dma, stm_driver->rx_dma->stream, (uint32_t) buff);
    dma_set_number_of_data(stm_driver->rx_dma->dma, stm_driver->rx_dma->stream, sz);
    dma_enable_transfer_complete_interrupt(stm_driver->rx_dma->dma, stm_driver->rx_dma->stream);
    
    //Interrupt when line is idle
    if (stm_driver->rx_end_cb != nullptr)
    {
      USART_CR1(stm_driver->uart_id) |= USART_CR1_IDLEIE;
    }
    
    stm_driver->rx_state = UartState::ACTIVE;

    dma_enable_stream(stm_driver->rx_dma->dma, stm_driver->rx_dma->stream);
  }

  inline void send(drivers::uart::UartDriver* driver, uint8_t* buff, const size_t sz)
  {
    auto stm_driver = reinterpret_cast<STM32UartDriver*>(driver); //TODO: Temp solution. Avoid downcasting

    dma_disable_stream(stm_driver->tx_dma->dma, stm_driver->tx_dma->stream);
    dma_set_memory_address(stm_driver->tx_dma->dma, stm_driver->tx_dma->stream, (uint32_t) buff);
    dma_set_number_of_data(stm_driver->tx_dma->dma, stm_driver->tx_dma->stream, sz);
    dma_enable_transfer_complete_interrupt(stm_driver->tx_dma->dma, stm_driver->tx_dma->stream);

    stm_driver->tx_state = UartState::ACTIVE;

    dma_enable_stream(stm_driver->tx_dma->dma, stm_driver->tx_dma->stream);
  }
}