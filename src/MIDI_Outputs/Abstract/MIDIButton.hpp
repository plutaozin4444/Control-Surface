#pragma once

#include <Def/Def.hpp>
#include <Hardware/Button.h>
#include <MIDI_Outputs/Abstract/MIDIOutputElement.hpp>

/**
 * @brief   An abstract class for momentary push buttons that send MIDI events.
 *
 * The button is debounced.
 *
 * @see     Button
 */
template <DigitalSendFunction sendOn, DigitalSendFunction sendOff>
class MIDIButton : public MIDIOutputElement {
  protected:
    /**
     * @brief   Construct a new MIDIButton.
     *
     * @param   pin
     *          The digital input pin with the button connected.
     *          The internal pull-up resistor will be enabled.
     */
    MIDIButton(pin_t pin, const MIDICNChannelAddress &address)
        : button{pin}, address{address} {}

  public:
    void begin() final override { button.begin(); }
    void update() final override {
        Button::State state = button.getState();
        if (state == Button::Falling) {
            sendOn(address);
        } else if (state == Button::Rising) {
            sendOff(address);
        }
    }

  private:
    Button button;
    const MIDICNChannelAddress address;
};