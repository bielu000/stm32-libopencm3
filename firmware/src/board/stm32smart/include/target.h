#pragma once 

extern "C" {
  #include <libopencm3/stm32/gpio.h>
  #include <libopencm3/cm3/nvic.h>
}

//Leds 
#define FAULT_LED_PORT       GPIOB
#define FAULT_LED_PIN        GPIO5
#define FAULT_LED_ACTIVE_ON  0 

#define STATUS_LED_PORT      GPIOA
#define STATUS_LED_PIN       GPIO11
#define STATUS_LED_ACTIVE_ON 0

#define OPERATIONAL_LED_PORT                GPIOA
#define OPERATIONAL_LED_PORT_LED_PIN        GPIO12
#define OPERATIONAL_LED_PORT_LED_ACTIVE_ON  0
//Leds end


//Encoder 
#define ENCODER_BUTTON_PORT                           GPIOA
#define ENCODER_BUTTON_PIN                            GPIO4
#define ENCODER_BUTTON_ISR_LINE                       NVIC_EXTI4_IRQ
#define ENCODER_BUTTON_EXTI                           EXTI4

#define ENCODER_CLOCKWISE_DIRECTION_PORT              GPIOB
#define ENCODER_CLOCKWISE_DIRECTION_PIN               GPIO11
#define ENCODER_CLOCKWISE_DIRECTION_ISR_LINE          NVIC_EXTI15_10_IRQ
#define ENCODER_CLOCKWISE_DIRECTION_EXTI              EXTI11

#define ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT      GPIOB
#define ENCODER_COUNTER_CLOCKWISE_DIRECTION_PIN       GPIO8
#define ENCODER_COUNTER_CLOCKWISE_DIRECTION_ISR_LINE  NVIC_EXTI9_5_IRQ
#define ENCODER_COUNTER_CLOCKWISE_DIRECTION_EXTI      EXTI8

//Encoder end
