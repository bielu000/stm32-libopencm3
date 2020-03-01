#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>
#include <led.h>
#include <target.h>
// #include <sys_timer.h>
#include <libopencm3/stm32/exti.h>
#include <delay.h>
#include <hd44780.h>
#include <libopencm3/stm32/i2c.h>
#include <ssd1306.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>
#include <hal/exti.h>
#include <rotary_encoder.h>
#include <stdlib.h>

static led_t fault_led;
static led_t operational_led;
static led_t status_led;
static sys_timer_t fault_led_timer;
static sys_timer_t operational_led_timer;
static sys_timer_t status_led_timer;

static void rcc_setup()
{
  //Clock setup
  rcc_clock_setup_in_hse_8mhz_out_72mhz();

  //Peripherals clock
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB); 
}

static void i2c_setup()
{
  //Required by hd44780 
  rcc_periph_clock_enable(RCC_I2C1);
  rcc_periph_clock_enable(RCC_AFIO);

  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C2_SCL | GPIO_I2C2_SDA);

  i2c_peripheral_disable(I2C2);
  i2c_set_clock_frequency(I2C2, I2C_CR2_FREQ_36MHZ);
  i2c_set_ccr(I2C2, 0x1e);
  i2c_set_trise(I2C2, 0x0b);
  i2c_set_own_7bit_slave_address(I2C2, 0x32);
  i2c_peripheral_enable(I2C2);
}

static void i2c_setup_I2C1()
{
  //Required by hd44780 
  rcc_periph_clock_enable(RCC_I2C2);
  rcc_periph_clock_enable(RCC_AFIO);

  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C1_SCL | GPIO_I2C1_SDA);

  i2c_peripheral_disable(I2C1);
  i2c_set_clock_frequency(I2C1, I2C_CR2_FREQ_36MHZ);
  i2c_set_ccr(I2C1, 0x1e);
  i2c_set_trise(I2C1, 0x0b);
  i2c_set_own_7bit_slave_address(I2C1, 0x32);
  i2c_peripheral_enable(I2C1);
}

static void systick_run()
{
  //Required by timers 
  systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);
  systick_set_reload(8999);
  systick_interrupt_enable();
  systick_counter_enable();
}

static void encoder_setup()
{
  //AFIO must be set 
  nvic_enable_irq(ENCODER_BUTTON_ISR_LINE);
  nvic_enable_irq(ENCODER_CLOCKWISE_DIRECTION_ISR_LINE);
  nvic_enable_irq(ENCODER_COUNTER_CLOCKWISE_DIRECTION_ISR_LINE);

  gpio_set_mode(ENCODER_BUTTON_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_BUTTON_PIN
  );
  gpio_set_mode(ENCODER_CLOCKWISE_DIRECTION_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_CLOCKWISE_DIRECTION_PIN
  );
  gpio_set_mode(ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_COUNTER_CLOCKWISE_DIRECTION_PIN
  );

  exti_select_source(ENCODER_BUTTON_EXTI, ENCODER_BUTTON_PORT);
  exti_set_trigger(ENCODER_BUTTON_EXTI, EXTI_TRIGGER_BOTH);
  exti_enable_request(ENCODER_BUTTON_EXTI);

  exti_select_source(ENCODER_CLOCKWISE_DIRECTION_EXTI, ENCODER_CLOCKWISE_DIRECTION_PORT);
  exti_set_trigger(ENCODER_CLOCKWISE_DIRECTION_EXTI, EXTI_TRIGGER_BOTH);
  exti_enable_request(ENCODER_CLOCKWISE_DIRECTION_EXTI);

  exti_select_source(ENCODER_COUNTER_CLOCKWISE_DIRECTION_EXTI, ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT);
  exti_set_trigger(ENCODER_COUNTER_CLOCKWISE_DIRECTION_EXTI, EXTI_TRIGGER_BOTH);
  exti_enable_request(ENCODER_COUNTER_CLOCKWISE_DIRECTION_EXTI);
}

void hal_init_gpio()
{
  gpio_set_mode(ENCODER_BUTTON_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_BUTTON_PIN
  );
  gpio_set_mode(ENCODER_CLOCKWISE_DIRECTION_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_CLOCKWISE_DIRECTION_PIN
  );
  gpio_set_mode(ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
    ENCODER_COUNTER_CLOCKWISE_DIRECTION_PIN
  );


  gpio_set(ENCODER_CLOCKWISE_DIRECTION_PORT, ENCODER_CLOCKWISE_DIRECTION_PIN);
  gpio_set(ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT, ENCODER_COUNTER_CLOCKWISE_DIRECTION_PIN);
}

void init_all_leds()
{
  fault_led.gpiox = FAULT_LED_PORT;
  fault_led.pin_mask = FAULT_LED_PIN;
  fault_led.active_on = FAULT_LED_ACTIVE_ON;

  status_led.gpiox = STATUS_LED_PORT;
  status_led.pin_mask = STATUS_LED_PIN;
  status_led.active_on = STATUS_LED_ACTIVE_ON;

  operational_led.gpiox = OPERATIONAL_LED_PORT;
  operational_led.pin_mask = OPERATIONAL_LED_PORT_LED_PIN;
  operational_led.active_on = OPERATIONAL_LED_PORT_LED_ACTIVE_ON;

  led_init(&fault_led);
  led_init(&status_led);
  led_init(&operational_led);

  led_toggle(&fault_led);
  led_toggle(&status_led);
  led_toggle(&operational_led);

  delay_ms(1000);

  led_toggle(&fault_led);
  led_toggle(&status_led);
  led_toggle(&operational_led);
}

void encoder_clockwise_handle()
{
  led_toggle(&fault_led);
}

void encoder_counter_clockwise_handle()
{
  led_toggle(&status_led);
}

void encoder_button_handle()
{
  led_toggle(&operational_led);
}

#include <stdio.h>

int main()
{
  rcc_setup();
  i2c_setup();

  sys_timers_init();
  systick_run();

  // encoder_setup();

  init_all_leds();
  hal_init_gpio();

  rotary_encoder_init();

  hal_exti_config_t cfg;
  HAL_EXTI_INIT_STRUCT(&cfg, ENCODER_BUTTON_PORT, ENCODER_BUTTON_PIN, 
    ENCODER_BUTTON_ISR_LINE);
  hal_exti_configure(&cfg); //configure push button for encoder

  HAL_EXTI_INIT_STRUCT(&cfg, ENCODER_CLOCKWISE_DIRECTION_PORT, ENCODER_CLOCKWISE_DIRECTION_PIN, 
    ENCODER_CLOCKWISE_DIRECTION_ISR_LINE);
  hal_exti_configure(&cfg); //configure clocwise direction for encoder 

  HAL_EXTI_INIT_STRUCT(&cfg, ENCODER_COUNTER_CLOCKWISE_DIRECTION_PORT, ENCODER_COUNTER_CLOCKWISE_DIRECTION_PIN, 
    ENCODER_COUNTER_CLOCKWISE_DIRECTION_ISR_LINE);
  hal_exti_configure(&cfg); //configure counter clocwise direction for encoder 

  rotary_encoder_set_callback(ROTARY_ENCODER_CLOCKWISE_ROTATION, encoder_clockwise_handle);
  rotary_encoder_set_callback(ROTARY_ENCODER_COUNTER_CLOCKWISE_ROTATION, encoder_counter_clockwise_handle);
  rotary_encoder_set_callback(ROTARY_ENCODER_BUTTON_PUSH, encoder_button_handle);

  i2c_setup_I2C1();

  hd44780_t lcd;
  lcd.i2cx = I2C1;
  lcd.dev_address = 0x27;
  lcd.busy_flag = false;
  hd44780_init(&lcd);

  char x[256];

  while(1)
  {
    int32_t counter = rotary_encoder_get_cnt();


    sprintf(x, "Counter %d", counter );
    hd44780_clear(&lcd);
    hd44780_display_text(&lcd, x);

  }
}