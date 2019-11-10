/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define LANGUAGE_PACKS_DEFINITION

#include "radio_setup.h"
#include "opentx.h"

#define SET_DIRTY()     storageDirty(EE_GENERAL)

class DateTimeWindow : public FormGroup {
  public:
    DateTimeWindow(FormGroup * parent, const rect_t & rect) :
      FormGroup(parent, rect, FORWARD_SCROLL | FORM_FORWARD_FOCUS)
    {
      build();
    }

    void checkEvents() override
    {
      FormGroup::checkEvents();

      if (get_tmr10ms() - lastRefresh > 100) {
        invalidate();
        lastRefresh = get_tmr10ms();
      }
    }

  protected:
    tmr10ms_t lastRefresh = 0;

    void build()
    {
      FormGridLayout grid;

      // Date
      new StaticText(this, grid.getLabelSlot(), STR_DATE);
      new NumberEdit(this, grid.getFieldSlot(3, 0), 2018, 2100,
                     [=]() -> int32_t {
                       struct gtm t;
                       gettime(&t);
                       return TM_YEAR_BASE + t.tm_year;
                     },
                     [=](int32_t newValue) {
                       struct gtm t;
                       gettime(&t);
                       t.tm_year = newValue - TM_YEAR_BASE;
                       SET_LOAD_DATETIME(&t);
                     });
      auto month = new NumberEdit(this, grid.getFieldSlot(3, 1), 1, 12,
                                  [=]() -> int32_t {
                                    struct gtm t;
                                    gettime(&t);
                                    return 1 + t.tm_mon;
                                  },
                                  [=](int32_t newValue) {
                                    struct gtm t;
                                    gettime(&t);
                                    t.tm_mon = newValue - 1;
                                    SET_LOAD_DATETIME(&t);
                                  });
      month->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        dc->drawNumber(FIELD_PADDING_LEFT, FIELD_PADDING_TOP, value, flags | LEADING0, 2);
      });

      /* TODO dynamic max instead of 31 ...
      int16_t year = TM_YEAR_BASE + t.tm_year;
      int8_t dlim = (((((year%4==0) && (year%100!=0)) || (year%400==0)) && (t.tm_mon==1)) ? 1 : 0);
      static const pm_uint8_t dmon[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
      dlim += *(&dmon[t.tm_mon]);*/
      int8_t dlim = 31;
      auto day = new NumberEdit(this, grid.getFieldSlot(3, 2), 1, dlim,
                                [=]() -> int32_t {
                                  struct gtm t;
                                  gettime(&t);
                                  return t.tm_mday;
                                },
                                [=](int32_t newValue) {
                                  struct gtm t;
                                  gettime(&t);
                                  t.tm_mday = newValue;
                                  SET_LOAD_DATETIME(&t);
                                });
      day->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        dc->drawNumber(FIELD_PADDING_LEFT, FIELD_PADDING_TOP, value, flags | LEADING0, 2);
      });
      grid.nextLine();

      // Time
      new StaticText(this, grid.getLabelSlot(), STR_TIME);
      auto hour = new NumberEdit(this, grid.getFieldSlot(3, 0), 0, 24,
                                 [=]() -> int32_t {
                                   struct gtm t;
                                   gettime(&t);
                                   return t.tm_hour;
                                 },
                                 [=](int32_t newValue) {
                                   struct gtm t;
                                   gettime(&t);
                                   t.tm_hour = newValue;
                                   SET_LOAD_DATETIME(&t);
                                 });
      hour->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        dc->drawNumber(FIELD_PADDING_LEFT, FIELD_PADDING_TOP, value, flags | LEADING0, 2);
      });

      auto minutes = new NumberEdit(this, grid.getFieldSlot(3, 1), 0, 59,
                                    [=]() -> int32_t {
                                      struct gtm t;
                                      gettime(&t);
                                      return t.tm_min;
                                    },
                                    [=](int32_t newValue) {
                                      struct gtm t;
                                      gettime(&t);
                                      t.tm_min = newValue;
                                      SET_LOAD_DATETIME(&t);
                                    });
      minutes->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        dc->drawNumber(FIELD_PADDING_LEFT, FIELD_PADDING_TOP, value, flags | LEADING0, 2);
      });

      auto seconds = new NumberEdit(this, grid.getFieldSlot(3, 2), 0, 59,
                                    [=]() -> int32_t {
                                      struct gtm t;
                                      gettime(&t);
                                      return t.tm_sec;
                                    },
                                    [=](int32_t newValue) {
                                      struct gtm t;
                                      gettime(&t);
                                      t.tm_sec = newValue;
                                      SET_LOAD_DATETIME(&t);
                                    });
      seconds->setDisplayHandler([](BitmapBuffer * dc, LcdFlags flags, int32_t value) {
        dc->drawNumber(FIELD_PADDING_LEFT, FIELD_PADDING_TOP, value, flags | LEADING0, 2);
      });

      grid.nextLine();
      getParent()->moveWindowsTop(top(), adjustHeight());
    }
};

RadioSetupPage::RadioSetupPage():
  PageTab(STR_RADIO_SETUP, ICON_RADIO_SETUP)
{
}

void RadioSetupPage::build(FormWindow * window)
{
  FormGridLayout grid;
  grid.spacer(PAGE_PADDING);

  // Date and Time
  auto timeWindow = new DateTimeWindow(window, {0, grid.getWindowHeight(), LCD_W, 0});
  grid.addWindow(timeWindow);

  // Batt meter range - Range 3.0v to 16v
  new StaticText(window, grid.getLabelSlot(), STR_BATTERY_RANGE);
  auto batMinEdit = new NumberEdit(window, grid.getFieldSlot(2, 0), -60 + 90, g_eeGeneral.vBatMax + 29 + 90, GET_SET_WITH_OFFSET(g_eeGeneral.vBatMin, 90), PREC1);
  batMinEdit->setSuffix("V");
  auto batMaxEdit = new NumberEdit(window, grid.getFieldSlot(2, 1), g_eeGeneral.vBatMin - 29 + 120, 40 + 120, GET_SET_WITH_OFFSET(g_eeGeneral.vBatMax, 120), PREC1);
  batMaxEdit->setSuffix("V");
  batMinEdit->setSetValueHandler([=](int32_t newValue) {
    g_eeGeneral.vBatMin= newValue - 90;
    SET_DIRTY();
    batMaxEdit->setMin(g_eeGeneral.vBatMin - 29 + 120);
    batMaxEdit->invalidate();
  });
  batMaxEdit->setSetValueHandler([=](int32_t newValue) {
    g_eeGeneral.vBatMax= newValue - 120;
    SET_DIRTY();
    batMinEdit->setMax(g_eeGeneral.vBatMax + 29 + 90);
    batMinEdit->invalidate();
  });
  grid.nextLine();

  new Subtitle(window, grid.getLabelSlot(), STR_SOUND_LABEL);
  grid.nextLine();

  // Beeps mode
  new StaticText(window, grid.getLabelSlot(true), STR_SPEAKER);
  new Choice(window, grid.getFieldSlot(), STR_VBEEPMODE, -2, 1, GET_SET_DEFAULT(g_eeGeneral.beepMode));
  grid.nextLine();

  // Main volume
  new StaticText(window, grid.getLabelSlot(true), STR_VOLUME);
  new Slider(window, grid.getFieldSlot(), -VOLUME_LEVEL_DEF, VOLUME_LEVEL_MAX-VOLUME_LEVEL_DEF, GET_SET_DEFAULT(g_eeGeneral.speakerVolume));
  grid.nextLine();

  // Beeps volume
  new StaticText(window, grid.getLabelSlot(true), STR_BEEP_VOLUME);
  new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.beepVolume));
  grid.nextLine();

  // Wav volume
  new StaticText(window, grid.getLabelSlot(true), STR_WAV_VOLUME);
  new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.wavVolume));
  grid.nextLine();

  // Background volume
  new StaticText(window, grid.getLabelSlot(true), STR_BG_VOLUME);
  new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.backgroundVolume));
  grid.nextLine();

  // Beeps length
  new StaticText(window, grid.getLabelSlot(true), STR_BEEP_LENGTH);
  new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.beepLength));
  grid.nextLine();

  // Beeps pitch
  new StaticText(window, grid.getLabelSlot(true), STR_BEEP_PITCH);
  auto edit = new NumberEdit(window, grid.getFieldSlot(), 0, 300,
                             GET_DEFAULT(15 * g_eeGeneral.speakerPitch),
                             [=](int32_t newValue) {
                               g_eeGeneral.speakerPitch = newValue / 15;
                               SET_DIRTY();
                             });
  edit->setStep(15);
  edit->setPrefix("+");
  edit->setSuffix("Hz");
  grid.nextLine();

#if defined(VARIO)
  {
    new Subtitle(window, grid.getLabelSlot(), STR_VARIO);
    grid.nextLine();

    // Vario volume
    new StaticText(window, grid.getLabelSlot(true), TR_VOLUME);
    new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.varioVolume));
    grid.nextLine();

    new StaticText(window, grid.getLabelSlot(true), STR_PITCH_AT_ZERO);
    edit = new NumberEdit(window, grid.getFieldSlot(), VARIO_FREQUENCY_ZERO - 400, VARIO_FREQUENCY_ZERO + 400,
                          GET_DEFAULT(VARIO_FREQUENCY_ZERO + (g_eeGeneral.varioPitch * 10)),
                          SET_VALUE(g_eeGeneral.varioPitch, (newValue - VARIO_FREQUENCY_ZERO) / 10));
    edit->setStep(10);
    edit->setSuffix("Hz");
    grid.nextLine();
  }
#endif

#if defined(HAPTIC)
  {
    new Subtitle(window, grid.getLabelSlot(), STR_HAPTIC_LABEL);
    grid.nextLine();

    // Haptic mode
    new StaticText(window, grid.getLabelSlot(true), STR_MODE);
    new Choice(window, grid.getFieldSlot(), STR_VBEEPMODE, -2, 1, GET_SET_DEFAULT(g_eeGeneral.hapticMode));
    grid.nextLine();

    // Haptic duration
    new StaticText(window, grid.getLabelSlot(true), STR_LENGTH);
    new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.hapticLength));
    grid.nextLine();

    // Haptic strength
    new StaticText(window, grid.getLabelSlot(true), STR_STRENGTH);
    new Slider(window, grid.getFieldSlot(), -2, +2, GET_SET_DEFAULT(g_eeGeneral.hapticStrength));
    grid.nextLine();
  }
#endif

  // Alarms
  {
    new Subtitle(window, grid.getLabelSlot(), STR_ALARMS_LABEL);
    grid.nextLine();

    // Battery warning
    new StaticText(window, grid.getLabelSlot(true), STR_BATTERYWARNING);
    edit = new NumberEdit(window, grid.getFieldSlot(), 30, 120, GET_SET_DEFAULT(g_eeGeneral.vBatWarn), PREC1);
    edit->setSuffix("v");
    grid.nextLine();

    // Alarms warning
    new StaticText(window, grid.getLabelSlot(true), STR_ALARMWARNING);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(g_eeGeneral.disableAlarmWarning));
    grid.nextLine();

    // RSSI shutdown alarm
    new StaticText(window, grid.getLabelSlot(true), STR_RSSI_SHUTDOWN_ALARM);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_INVERTED(g_eeGeneral.disableRssiPoweroffAlarm));
    grid.nextLine();

    // Inactivity alarm
    new StaticText(window, grid.getLabelSlot(true), STR_INACTIVITYALARM);
    edit = new NumberEdit(window, grid.getFieldSlot(), 0, 250, GET_SET_DEFAULT(g_eeGeneral.inactivityTimer));
    edit->setSuffix("minutes");
    grid.nextLine();
  }

  // Backlight
  {
    new Subtitle(window, grid.getLabelSlot(), STR_BACKLIGHT_LABEL);
    grid.nextLine();

    // Backlight mode
    new StaticText(window, grid.getLabelSlot(true), STR_MODE);
    new Choice(window, grid.getFieldSlot(2,0), STR_VBLMODE, e_backlight_mode_off, e_backlight_mode_on, GET_SET_DEFAULT(g_eeGeneral.backlightMode));
    //grid.nextLine();

    // Delay
    auto edit = new NumberEdit(window, grid.getFieldSlot(2, 1), 0, 600,
                               GET_DEFAULT(g_eeGeneral.lightAutoOff * 5),
                               SET_VALUE(g_eeGeneral.lightAutoOff, newValue / 5));
    edit->setStep(5);
    edit->setSuffix("s");
    grid.nextLine();

    // Backlight ON bright
    new StaticText(window, grid.getLabelSlot(true), STR_BLONBRIGHTNESS);
    new Slider(window, grid.getFieldSlot(), BACKLIGHT_LEVEL_MIN, BACKLIGHT_LEVEL_MAX,
               [=]() -> int32_t {
                 return BACKLIGHT_LEVEL_MAX - g_eeGeneral.backlightBright;
               },
               [=](int32_t newValue) {
                 g_eeGeneral.backlightBright = BACKLIGHT_LEVEL_MAX - newValue;
               });
    grid.nextLine();

    // Backlight OFF bright
    new StaticText(window, grid.getLabelSlot(true), STR_BLOFFBRIGHTNESS);
    new Slider(window, grid.getFieldSlot(), BACKLIGHT_LEVEL_MIN, BACKLIGHT_LEVEL_MAX, GET_SET_DEFAULT(g_eeGeneral.blOffBright));
    grid.nextLine();

    // Flash beep
    new StaticText(window, grid.getLabelSlot(true), STR_ALARM);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_eeGeneral.alarmsFlash));
    grid.nextLine();
  }

#if defined(PWR_BUTTON_PRESS)
  {
    new StaticText(window, grid.getLabelSlot(), STR_PWR_OFF_DELAY);
    auto edit = new NumberEdit(window, grid.getFieldSlot(), 0, 3,
                               [=]() -> int32_t {
                                 return 2 - g_eeGeneral.pwrOffSpeed;
                               },
                               [=](int32_t newValue) {
                                 g_eeGeneral.pwrOffSpeed = 2 - newValue;
                               });
    edit->setSuffix("s");
    grid.nextLine();
  }
#endif
  
  // GPS
  {
    new Subtitle(window, grid.getLabelSlot(), STR_GPS);
    grid.nextLine();

    // Timezone
    new StaticText(window, grid.getLabelSlot(true), STR_TIMEZONE);
    new NumberEdit(window, grid.getFieldSlot(2, 0), -12, 12, GET_SET_DEFAULT(g_eeGeneral.timezone));
    grid.nextLine();

    // Adjust RTC (from telemetry)
    new StaticText(window, grid.getLabelSlot(true), STR_ADJUST_RTC);
    new CheckBox(window, grid.getFieldSlot(), GET_SET_DEFAULT(g_eeGeneral.adjustRTC));
    grid.nextLine();

    // GPS format
    new StaticText(window, grid.getLabelSlot(true), STR_GPS_COORDS_FORMAT);
    new Choice(window, grid.getFieldSlot(), STR_GPSFORMAT, 0, 1, GET_SET_DEFAULT(g_eeGeneral.gpsFormat));
    grid.nextLine();
  }

#if defined(PXX2)
  // Owner ID
  new StaticText(window, grid.getLabelSlot(), STR_OWNER_ID);
  new TextEdit(window, grid.getFieldSlot(), g_eeGeneral.ownerRegistrationID, PXX2_LEN_REGISTRATION_ID);
  grid.nextLine();
#endif

  // Country code
  new StaticText(window, grid.getLabelSlot(), STR_COUNTRY_CODE);
  new Choice(window, grid.getFieldSlot(), STR_COUNTRY_CODES, 0, 2, GET_SET_DEFAULT(g_eeGeneral.countryCode));
  grid.nextLine();

  // Audio language
  new StaticText(window, grid.getLabelSlot(), STR_VOICE_LANGUAGE);
  auto choice = new Choice(window, grid.getFieldSlot(), nullptr, 0, DIM(languagePacks) - 2, GET_VALUE(currentLanguagePackIdx),
                           [](uint8_t newValue) {
                             currentLanguagePackIdx = newValue;
                             currentLanguagePack = languagePacks[currentLanguagePackIdx];
                             strncpy(g_eeGeneral.ttsLanguage, currentLanguagePack->id, 2);
                           });
  choice->setTextHandler([](uint8_t value) {
    return languagePacks[value]->name;
  });
  grid.nextLine();

  // Imperial units
  new StaticText(window, grid.getLabelSlot(), STR_UNITS_SYSTEM);
  new Choice(window, grid.getFieldSlot(), STR_VUNITSSYSTEM, 0, 1, GET_SET_DEFAULT(g_eeGeneral.imperial));
  grid.nextLine();

#if defined(FAI_CHOICE)
/*  case ITEM_SETUP_FAI:
    lcdDrawText(MENUS_MARGIN_LEFT, y, "FAI Mode");
    if (g_eeGeneral.fai) {
      lcdDrawText(RADIO_SETUP_2ND_COLUMN, y, "Locked in FAI Mode");
    }
    else {
      g_eeGeneral.fai = editCheckBox(g_eeGeneral.fai, RADIO_SETUP_2ND_COLUMN, y, attr, event);
      if (attr && checkIncDec_Ret) {
          g_eeGeneral.fai = false;
          POPUP_CONFIRMATION("FAI mode?");
      }
    }
    break;*/
#endif

  // Switches delay
  new StaticText(window, grid.getLabelSlot(), STR_SWITCHES_DELAY);
  edit = new NumberEdit(window, grid.getFieldSlot(2, 0), -15, 100 - 15, GET_SET_VALUE_WITH_OFFSET(g_eeGeneral.switchesDelay, 15));
  edit->setSuffix(std::string("0") + STR_MS);
  grid.nextLine();

  // USB mode
  new StaticText(window, grid.getLabelSlot(), STR_USBMODE);
  new Choice(window, grid.getFieldSlot(), STR_USBMODES, USB_UNSELECTED_MODE, USB_MAX_MODE, GET_SET_DEFAULT(g_eeGeneral.USBMode));
  grid.nextLine();

  // RX channel order
  new StaticText(window, grid.getLabelSlot(), STR_RXCHANNELORD); // RAET->AETR
  choice = new Choice(window, grid.getFieldSlot(), nullptr, 0, 4*3*2 - 1, GET_SET_DEFAULT(g_eeGeneral.templateSetup));
  choice->setTextHandler([](uint8_t value) {
    char s[5];
    for (uint8_t i=0; i<4; i++) {
      s[i] = STR_RETA123[channelOrder(value, i + 1)];
    }
    s[4] = '\0';
    return std::string(s);
  });
  grid.nextLine();

  // Stick mode
  new StaticText(window, grid.getLabelSlot(), STR_MODE);
  choice = new Choice(window, grid.getFieldSlot(), nullptr, 0, 3, GET_DEFAULT(g_eeGeneral.stickMode),
                      [=](uint8_t newValue) {
                        pausePulses();
                        g_eeGeneral.stickMode = newValue;
                        SET_DIRTY();
                        checkThrottleStick();
                        resumePulses();
                      });
  choice->setTextHandler([](uint8_t value) {
    return std::to_string(1 + value) + ": left=" + std::string(&getSourceString(MIXSRC_Rud + modn12x3[4 * value])[1]) + "+" + std::string(&getSourceString(MIXSRC_Rud + modn12x3[4 * value + 1])[1]);
  });
  grid.nextLine();

  window->setInnerHeight(grid.getWindowHeight());
}
