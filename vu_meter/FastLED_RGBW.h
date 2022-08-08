/* FastLED_RGBW
 *
 * Hack to enable SK6812 RGBW strips to work with FastLED.
 *
 * Original code by Jim Bumgardner (http://krazydad.com).
 * Modified by David Madison (http://partsnotincluded.com).
 * Modified by Christof Kaufmann.
 *
*/

// See: https://www.partsnotincluded.com/fastled-rgbw-neopixels-sk6812/
// See: https://www.dmurph.com/posts/2021/1/cabinet-light-3.html
#define SWAP(x, y) do { x = x^y; y = x^y; x = x^y;} while (0)

#pragma once

#include <FastLED.h>
#include <memory> // for std::array in CRGBWArray

/**
 * @brief Extends CRGB by white
 *
 * This contains RGB values and additionally a value for white. This allows to
 * use RGBW LEDs with FastLED, but not perfectly. Things like color or
 * temperature correction won't work, since it is applied right before sending.
 * But this workaround does not give FastLED real RGBW capabilities, so to be
 * clear: FastLED still thinks that it is sending color data to RGB LEDs:
 * @code
 * FastLED sees:  R G B|R G B|R G B|R G B|R G B|R G B|
 * True LEDs are: R G B W|R G B W|R G B W|R G B W|
 * @endcode
 */
struct CRGBW : public CRGB {
  union {
    uint8_t w;
    uint8_t white;
  };

  CRGBW() : CRGB(0, 0, 0), w{0} {}

  CRGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0)
    : CRGB(r, g, b), w{w}
  {}

  inline void operator=(const CRGB c) __attribute__((always_inline)){
    this->r = c.r;
    this->g = c.g;
    this->b = c.b;
    this->w = 0;
  }

  /**
   * @brief Blend two colors in screen mode
   * 
   * This blends two colors nicely giving a bright color.
   * Alpha is not supported here, since CRGBW does not support it.
   * 
   * @see https://en.wikipedia.org/wiki/Blend_modes
   */
  CRGBW& screen_blend(CRGB b) { // parameter type must be `CRGB` instead of `CRGB const&` due to a bug in operator-
    CRGB complement = -(*this);
    *this = -(complement.scale8(-b));
    return *this;
  }
};

CRGB blend(CRGBW a, CRGB const& b) {
  return a.screen_blend(b);
}

using CRGBWSet = CPixelView<CRGBW>;

__attribute__((always_inline))
inline CRGBW* operator+(CRGBWSet const& pixels, int offset) {
  return (CRGBW*)pixels + offset;
}

/**
 * @brief Number of RGB LEDs required to contain the RGBW LEDs values
 *
 * For details, see #CRGBWArray::rgb_size()
 */
constexpr int rgb_size_from_rgbw_size(int nleds_rgbw){ // original name: getRGBWsize
  return nleds_rgbw * 4 / 3 + (nleds_rgbw % 3 != 0);
}

/**
 * @brief Calibration factors of the white LED
 *
 * The values here mean if white is set to 100 (and red, green and blue are
 * off), it has the same color and brightness as if red is set to 255, green to
 * 140 and blue to 25 (and white is off).
 *
 * To find these values, start by trying to match the color of white and then
 * brightness. The brightness is hard to estimate, so looking at converted
 * colors might help.
 */
constexpr float whitepoint_red_factor   = 100. / 255.;
constexpr float whitepoint_green_factor = 100. / 140.; ///< @copydoc whitepoint_red_factor
constexpr float whitepoint_blue_factor  = 100. / 25.;  ///< @copydoc whitepoint_red_factor

/**
 * @brief Convert the RGB values to RGBW values in send order
 *
 * For details, see #CRGBWArray::convert_to_rgbw()
 */
void rgb2rgbw(CRGBW leds[], size_t n_leds) {
  for (uint16_t idx = 0; idx < n_leds; ++idx) {
    CRGBW& led = leds[idx];
    if (led.w > 0)
      continue;

    // These values are what the 'white' value would need to be to get the corresponding color value.
    float white_val_from_red   = led.r * whitepoint_red_factor;
    float white_val_from_green = led.g * whitepoint_green_factor;
    float white_val_from_blue  = led.b * whitepoint_blue_factor;

    // Set the white value to the highest it can be for the given color
    // (without over saturating any channel - thus the minimum of them).
    float min_white_value = min(white_val_from_red, std::min(white_val_from_green, white_val_from_blue));
    // Don't use transformation for very dark values, since rounding errors are visible there
    if (min_white_value > 4) {
      // The rest of the channels will just be the original value minus the contribution by the white channel.
      led.r = static_cast<uint8_t>(round(led.r - led.w / whitepoint_red_factor));
      led.g = static_cast<uint8_t>(round(led.g - led.w / whitepoint_green_factor));
      led.b = static_cast<uint8_t>(round(led.b - led.w / whitepoint_blue_factor));
      led.w = static_cast<uint8_t>(floor(min_white_value));
    }

    // GRBW send order must be taken into account here and set outside to RGB
    SWAP(led.r, led.g);
  }
}

/// @copydoc rgb2rgbw
template <class LEDArray>
void rgb2rgbw(LEDArray& leds) {
  rgb2rgbw(&leds[0], leds.size());
}

/**
 * @brief Convert RGBW values back to RGB
 *
 * This converts the RGBW values back to RGB in case you want to modify them
 * instead of completely overwriting them. It also accounts for the send order
 * by SWAPping back red and green values.
 */
void rgbw2rgb(CRGBW leds[], size_t n_leds) {
  for (uint16_t idx = 0; idx < n_leds; ++idx) {
    CRGBW& led = leds[idx];

    // Reverse send order here
    CRGBW temp = led;
    SWAP(temp.r, temp.g);

    // Reverse transformation
    led.r = static_cast<uint8_t>(max(255.0f, round(temp.r + temp.w / whitepoint_red_factor)));
    led.g = static_cast<uint8_t>(max(255.0f, round(temp.g + temp.w / whitepoint_green_factor)));
    led.b = static_cast<uint8_t>(max(255.0f, round(temp.b + temp.w / whitepoint_blue_factor)));
    led.w = 0;
  }
}

/// @copydoc rgbw2rgb
template <class LEDArray>
void rgbw2rgb(LEDArray& leds) {
  rgbw2rgb(&leds[0], leds.size());
}

/**
 * @brief CRGBW Array class
 *
 * Here is an example of how to use it. First, create the array as global
 * variable:
 * @code
 * constexpr int NUM_LEDS = 87;
 * CRGBWArray<NUM_LEDS> leds;
 * @endcode
 *
 * Then, configure FastLED to use it (e. g. in the Arduino `setup()` function):
 * @code
 * constexpr int LED_IO = 19;
 * FastLED.addLeds<SK6812, LED_IO, RGB>(leds, leds.rgb_size());
 * @endcode
 * Note, that the send order must be RGB, although the real send order is GRBW!
 * After drawing your effects into `leds`, you have to convert the colors to
 * RGBW, which also handles the send order (e. g. in the Arduino `loop()`
 * function):
 * @code
 * leds.convert_to_rgbw();
 * FastLED.delay(100); // send during delay
 * @endcode
 */
template<int SIZE>
class CRGBWArray : public CPixelView<CRGBW> {
    std::array<CRGBW, SIZE> rawleds;

  public:
    CRGBWArray() : CPixelView<CRGBW>(rawleds.data(), SIZE) {}
    using CPixelView::operator=;

    /**
     * @brief Number of RGB LEDs required to contain the RGBW LEDs values
     *
     * This is useful for the `FastLED.addLeds()` call, see the example at
     * #CRGBWArray.
     *
     * Example: For 10 RGBW LEDs you need 40 bytes color data, which fits into
     * 14 RGB LEDs (42 bytes).
     */
    static constexpr int rgb_size() {
      return rgb_size_from_rgbw_size(SIZE);
    }

    /**
     * @brief Convert this Array to a CRGB pointer onto the first element
     *
     * This is useful for the `FastLED.addLeds()` call.
     */
    inline operator CRGB* () {
      return reinterpret_cast<CRGB*>(rawleds.data());
    }

    /**
     * @brief Convert the RGB values to RGBW values in send order
     *
     * The part in the RGB values, that can be represented by the white led is
     * removed from the RGB values and set to the white value.
     * 
     * Using the equivalent RGBW values reduces power consumption and enhances
     * the light quality (RA value). However, it does not allow to go beyond
     * the brightness of full RGB. You could set the white value afterwards if
     * you really want.
     *
     * Besides it SWAPs the red and green channel in the converted values to
     * handle the send order.
     */
    inline void convert_to_rgbw() {
      rgb2rgbw(*this);
    }

    /**
     * @brief Convert the RGBW values in send order back to RGB values
     *
     * This SWAPs the red and green channel to revert the send order fix and
     * then adds the white led value split into RGB color components back to
     * the remained RGB values. This sets the white value to 0.
     * 
     * This function is useful, if you want to modify the led values directly
     * in the CRGBWArray.
     */
    inline void revert_to_rgb() {
      rgbw2rgb(*this);
    }
};
