//
// Created by pp on 03.03.20.
//

#include <stdlib.h>
#include <string.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <gps.h>

#define CR 13
#define LF 10
#define NULL_TERM 0
#define DELIMITER ","

char* strtoke(char *str, const char *delim)
{
  static char *start = NULL; /* stores string str for consecutive calls */
  char *token = NULL; /* found token */
  if (str) start = str;
  if (!start) return NULL;
  token = start;
  start = strpbrk(start, delim);
  if (start) *start++ = '\0';
  return token;
}

// PRIVATE FUNCTIONS DECLARATIONS //
static void __gps_init_usart_driver();
static void __gps_parse_line(gps_handle_t gps);
static void __gps_parse_gprmc(gps_handle_t gps);
static void __gps_parse_gpvtg(gps_handle_t gps);
static void __gps_parse_gpgga(gps_handle_t gps);
static void __gps_parse_gpgsa(gps_handle_t gps);
static void __gps_parse_gpgsv(gps_handle_t gps);
static void __gps_parse_gpgll(gps_handle_t gps);
static void __gps_load_next_line(gps_handle_t gps);

static uint8_t working_buffer_data[GPS_WORKING_BUFFER_SIZE];

void gps_init(gps_handle_t gps_handle)
{
  __gps_init_usart_driver();

  ring_buffer_init(&gps_handle->working_buffer,
    working_buffer_data,
    GPS_WORKING_BUFFER_SIZE,
    RING_BUFFER_OPTION_ALLOW_OVERLAPPING
  );

  gps_handle->lines_of_data = 0;

  //Data - time
  gps_handle->hour = 0;
  gps_handle->minute = 0;
  gps_handle->second = 0;

  //Speed
  gps_handle->speed_knots = 0;
  gps_handle->speed_kilomiters = 0;

  //Altitude, satelites, quality
  gps_handle->quality = GPS_NO_FIX;
  gps_handle->altitude = 0;
  gps_handle->satelites_in_use = 0;

  //Mode, pdop, gdop, vdop
  gps_handle->mode = 0;
  gps_handle->pdop = 0;
  gps_handle->hdop = 0;
  gps_handle->vdop = 0;
}

void gps_receive_char(gps_handle_t gps, char c)
{
  if (c == LF || c == NULL_TERM) {
    return;
  }

  if (c == CR) {
    gps->lines_of_data++;
  }

  rbuff_handle_t rbuff_handle = &gps->working_buffer;

  ring_buffer_write(rbuff_handle, (uint8_t)c);
}

void gps_task(gps_handle_t gps)
{
  if (gps->lines_of_data)
  {
    __gps_parse_line(gps);
    __gps_load_next_line(gps);
  }
}

// PRIVATE FUNCTIONS IMPLEMENTATION //
void __gps_init_usart_driver()
{
  //TODO: Change usart configuration in order to not hardcode e.g. usart type

  nvic_enable_irq(NVIC_USART1_IRQ);

  gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, 
    GPIO9);

  rcc_periph_clock_enable(RCC_USART1);

  usart_disable(USART1);
  usart_set_baudrate(USART1, 9600);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  usart_set_mode(USART1, USART_MODE_RX);

  usart_enable_rx_interrupt(USART1);
  usart_enable(USART1);
}

void __gps_parse_line(gps_handle_t gps)
{
  rbuff_handle_t rbuff_h = &gps->working_buffer;

  char* token;
  char* bufptr = (char*)&(rbuff_h->data[ring_buffer_tail(rbuff_h)]);
  token = strtoke(bufptr, DELIMITER);

  if (strcmp(token, "$GPRMC") == 0) __gps_parse_gprmc(gps);
  else if (strcmp(token, "$GPVTG") == 0) __gps_parse_gpvtg(gps);
  else if (strcmp(token, "$GPGGA") == 0) __gps_parse_gpgga(gps);
  else if (strcmp(token, "$GPGSA") == 0) __gps_parse_gpgsa(gps);
  else if (strcmp(token, "$GPGSV") == 0) __gps_parse_gpgsv(gps);
  else if (strcmp(token, "$GPGLL") == 0) __gps_parse_gpgll(gps);
}

void __gps_load_next_line(gps_handle_t gps)
{
  rbuff_t* rbuff = &gps->working_buffer;
  uint8_t c = 0;

  do {
    ring_buffer_read(rbuff, &c);
  } while(c != CR);

  gps->lines_of_data--;
}

void __gps_parse_gprmc(gps_handle_t gps)
{
  char* token;
  uint32_t tmp;

  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0) {
    tmp = atoi(token);

    gps->second = (uint8_t)(tmp % 100);
    gps->minute = (uint8_t)((tmp / 100) % 100);
    gps->hour = (uint8_t)((tmp / 10000) % 100);
  }
}

void __gps_parse_gpvtg(gps_handle_t gps)
{
  char* token;

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);

  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0)
  {
    gps->speed_knots = atof(token);
  }

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0)
  {
    gps->speed_kilomiters = atof(token);
  }
}

void __gps_parse_gpgga(gps_handle_t gps)
{
  char* token;

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
  if (strlen(token) > 0) 
  {
    gps->latitude = atof(token);
  }

  token = strtoke(NULL, DELIMITER);
  if (strlen(token) > 0)
  {
    gps->latitude_direction = token[0];
  }

  token = strtoke(NULL, DELIMITER); 
  if (strlen(token) > 0)
  {
      gps->longitude = atof(token);
  }

  token = strtoke(NULL, DELIMITER);
  if (strlen(token) > 0)
  {
      gps->longitude_direction = token[0];
  }

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0)
  {
    gps->quality = (uint8_t)atoi(token);
  }

  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0)
  {
    gps->satelites_in_use = (uint8_t)atoi(token);
  }

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);

  if (strlen(token) > 0)
  {
    gps->altitude = atof(token);
  }

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);
}

void __gps_parse_gpgsa(gps_handle_t gps)
{
  char* token;

  token = strtoke(NULL, DELIMITER);
  token = strtoke(NULL, DELIMITER);

  //Mode
  if (strlen(token) > 0)
  {
    gps->mode = (uint8_t)atoi(token);
  }

  //skip ids of SVs
  for (uint8_t i = 0; i < 12; i++)
  {
    token = strtoke(NULL, DELIMITER);
  }

  //PDOP
  token = strtoke(NULL, DELIMITER);
  if (strlen(token) > 0)
  {
    gps->pdop = atof(token);
  }

  //HDOP
  token = strtoke(NULL, DELIMITER);
  if (strlen(token) > 0)
  {
    gps->hdop = atof(token);
  }

  //VDOP
  token = strtoke(NULL, DELIMITER);
  {
    gps->vdop = atof(token);
  }
}

void __gps_parse_gpgsv(gps_handle_t gps)
{
  //Not implemented, because these informations are not needed.

  //  eg. with fix:
  //    - $GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74
  //    - $GPGSV,3,2,11,14,25,170,00,16,57,208,39,18,67,296,40,19,40,246,00*74
  //    - $GPGSV,3,3,11,22,42,067,42,24,14,311,43,27,05,244,00,,,,*4D
  //    - $GPGSV,1,1,13,02,02,213,,03,-3,000,,11,00,121,,14,13,172,05*67
  //
  //  eg. without fix
  //  $GPGSV,1,1,01,09,,,27*74
  //
  //  1    = Total number of messages of this type in this cycle
  //  2    = Message number
  //  3    = Total number of SVs in view
  //  4    = SV PRN number
  //  5    = Elevation in degrees, 90 maximum
  //  6    = Azimuth, degrees from true north, 000 to 359
  //  7    = SNR, 00-99 dB (null when not tracking)
  //  8-11 = Information about second SV, same as field 4-7
  //  12-15= Information about third SV, same as field 4-7
  //  16-19= Information about fourth SV, same as field 4-7
}

void __gps_parse_gpgll(gps_handle_t gps)
{
  // Not implemented, because these information is also provided by GPGRMC
  // eg. with fix:
  //  - $GPGLL,3751.65,S,14507.36,E*77
  //  - $GPGLL,4916.45,N,12311.12,W,225444,A
  //
  //
  //           4916.46,N    Latitude 49 deg. 16.45 min. North
  //           12311.12,W   Longitude 123 deg. 11.12 min. West
  //           225444       Fix taken at 22:54:44 UTC
  //           A            Data valid
  //
  //
  //  - eg3. $GPGLL,5133.81,N,00042.25,W*75
  //               1    2     3    4 5
  //
  // eg. without fix:
  //   - $GPGLL,,,,,,V,N*64
  //
  //  1    5133.81   Current latitude
  //  2    N         North/South
  //  3    00042.25  Current longitude
  //  4    W         East/West
  //  5    *75       checksum
}