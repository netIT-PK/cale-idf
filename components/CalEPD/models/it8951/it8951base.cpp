#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"
#include "it8951/it8951base.h"

// Plasticlogic will replace Epd as baseclass for this models and have all common methods for all EPDs of this manufacturer
void It8951Base::initIO(bool debug) {
  IO.init(11, debug); // Gx uses 24 MHz frequency. 10 Mhz for read
  
  if (CONFIG_EINK_RST > -1) {
    printf("IO.reset(Long)\n");
    IO.reset(244);
  }
  
}

void It8951Base::_InitDisplay() {
   // Long reset pulse
   printf("_InitDisplay\n");
   IO.reset(200);
   _waitBusy("Reset", reset_to_ready_time);
}

void It8951Base::_powerOff() {
  _IT8951StandBy();
  _waitBusy("_PowerOff", power_off_time);
  _power_is_on = false;
}

void It8951Base::_powerOn() {
  if (!_power_is_on)
  {
    _IT8951SystemRun();
    _waitBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void It8951Base::_IT8951WriteReg(uint16_t usRegAddr, uint16_t usValue)
{
  //Send Cmd , Register Address and Write Value
  _writeCommand16(IT8951_TCON_REG_WR);
  _writeData16(usRegAddr);
  _writeData16(usValue);
}

void It8951Base::_writeData16(uint16_t d)
{
  _waitBusy("_writeData16", default_wait_time);
  uint8_t d1 = d >>8 & 0xff;
  uint8_t d2 = d & 0xff;
  uint8_t t16[4] = {0x00, 0x00, d1, d2};
  IO.data(t16, sizeof(t16));
}

void It8951Base::_writeCommand16(uint16_t c)
{
  char cmd[10];
  sprintf(cmd, "cmd(%02x)", c);
  _waitBusy(cmd, default_wait_time);

  // 0x6000 preamble for command
  // 60 00 c0 c1
  uint8_t c1 = c >>8 & 0xff;
  uint8_t c2 = c  & 0xff;
  uint8_t t16[4] = {0x60, 0x00, c1, c2};
  IO.data(t16, sizeof(t16));

}

void It8951Base::_writeCommandData16(uint16_t c, const uint16_t* d, uint16_t n)
{
  char cmd[15];
  // 0x6000 preamble for command
  uint8_t c1 = c >>8 & 0xff;
  uint8_t c2 = c & 0xff;
  sprintf(cmd, "cmdD(%x%x)", c, c);
  _waitBusy(cmd, default_wait_time);

  uint8_t t16[4+n] = {0x60, 0x00, c1, c2};
  
  for (uint64_t i = 4; i < n+4; i++) {
    t16[i] = d[i-4];
  }
  IO.data(t16, sizeof(t16));
}

void It8951Base::_IT8951SystemRun()
{
  _writeCommand16(IT8951_TCON_SYS_RUN);
}

void It8951Base::_IT8951StandBy()
{
  _writeCommand16(IT8951_TCON_STANDBY);
}

void It8951Base::_IT8951Sleep()
{
  _writeCommand16(IT8951_TCON_SLEEP);
}


void It8951Base::_waitBusy(const char* message, uint16_t busy_time){
  //if (debug_enabled) {
    ESP_LOGI(TAG, "_BUSY for %s", message);
    // Add some margin
    vTaskDelay(1 / portTICK_RATE_MS);
  //}


  int64_t time_since_boot = esp_timer_get_time();
  // On low is busy
  if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 0) {
   
  while (1){
    if (gpio_get_level((gpio_num_t)CONFIG_EINK_BUSY) == 1) {
      printf(" waited:%lld ms\n", (esp_timer_get_time()-time_since_boot)/1000);
      break;
    }
    vTaskDelay(1 / portTICK_RATE_MS);
    uint64_t timespent = (esp_timer_get_time()-time_since_boot)/1000;

    if (timespent>200)
    {
      ESP_LOGI(TAG, "Busy Timeout>%d ts:%lld", busy_time, timespent);
      break;
    }
  }

  } else {
    vTaskDelay(busy_time / portTICK_RATE_MS);
    printf("HRDY is high\n");
  }
}

void It8951Base::_IT8951SetVCOM(uint16_t vcom)
{
  _writeCommand16(USDEF_I80_CMD_VCOM);
  _waitBusy("_IT8951SetVCOM", default_wait_time);
  _writeData16(1);
  //Read data from Host Data bus
  _writeData16(vcom);
  _waitBusy("_IT8951SetVCOM", set_vcom_time);
}

// GFX functions
// display.print / println handling .TODO: Implement printf
size_t It8951Base::write(uint8_t v){
  Adafruit_GFX::write(v);
  return 1;
}
uint8_t It8951Base::_unicodeEasy(uint8_t c) {
  if (c<191 && c>131 && c!=176) { // 176 is °W 
    c+=64;
  }
  return c;
}

void It8951Base::print(const std::string& text){
   for(auto c : text) {
     if (c==195 || c==194) continue; // Skip to next letter
     c = _unicodeEasy(c);
     write(uint8_t(c));
   }
}

void It8951Base::println(const std::string& text){
   for(auto c : text) {
     if (c==195 || c==194) continue; // Skip to next letter

     // _unicodeEasy will just sum 64 and get the right character when using umlauts and other characters:
     c = _unicodeEasy(c);
     write(uint8_t(c));
   }
   write(10); // newline
}

void It8951Base::newline() {
  write(10);
}

//-----------------------------------------------------------
//Host Cmd 10---LD_IMG
//-----------------------------------------------------------
void It8951Base::loadImgStart(IT8951LdImgInfo* pstLdImgInfo)
{
    uint16_t usArg;
    //Setting Argument for Load image start
    usArg = (pstLdImgInfo->usEndianType << 8 )
    |(pstLdImgInfo->usPixelFormat << 4)
    |(pstLdImgInfo->usRotate);

    //Send Cmd
    _writeCommand16(IT8951_TCON_LD_IMG);
    //Send Arg
    _writeData16(usArg);
}
