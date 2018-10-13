#pragma once

#include "Bank.h"
#include "BankConfig.hpp"
#include <Helpers/MIDICNChannelAddress.hpp>

class BankableMIDIOutput {
  protected:
    BankableMIDIOutput(const OutputBank &bank, BankType type)
        : bank(bank), type(type) {}

    BankableMIDIOutput(const OutputBankConfig &config)
        : BankableMIDIOutput(config.bank, config.type) {}

  public:
    MIDICNChannelAddress getAddressOffset() const {
        int8_t selection = getSelection();
        switch (type) {
            case CHANGE_ADDRESS: return {selection, Channel{0}, 0};
            case CHANGE_CHANNEL: return {0, Channel{selection}, 0};
            case CHANGE_CN: return {0, Channel{0}, selection};
            default: return {};
        }
    }

    setting_t getRawBankSetting() const { return bank.getSelection(); }

    setting_t getSelection() const {
        return lockedSetting == UNLOCKED ? getRawBankSetting() : lockedSetting;
    }

  protected:
    void lock() {
        if (lockedSetting == UNLOCKED)
            lockedSetting = getRawBankSetting();
    }

    void unlock() { lockedSetting = UNLOCKED; }

  private:
    const OutputBank &bank;
    const BankType type;

    constexpr static setting_t UNLOCKED = NO_SETTING;
    setting_t lockedSetting = UNLOCKED;
};