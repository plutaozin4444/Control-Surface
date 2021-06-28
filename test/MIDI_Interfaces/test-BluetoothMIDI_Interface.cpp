#include <MIDI_Interfaces/BluetoothMIDI_Interface.hpp>
#include <MIDI_Interfaces/MIDI_Callbacks.hpp>

using namespace CS;
using testing::Mock;

class MockMIDI_Callbacks : public MIDI_Callbacks {
  public:
    void onChannelMessage(MIDI_Interface &, ChannelMessage msg) override {
        channelMessages.push_back(msg);
    }
    void onSysExMessage(MIDI_Interface &, SysExMessage msg) override {
        sysExMessages.insert(sysExMessages.end(), msg.data,
                             msg.data + msg.length);
        sysExCounter++;
    }
    void onRealTimeMessage(MIDI_Interface &, RealTimeMessage msg) override {
        realtimeMessages.push_back(msg);
    }

    std::vector<ChannelMessage> channelMessages;
    std::vector<uint8_t> sysExMessages;
    std::vector<RealTimeMessage> realtimeMessages;
    size_t sysExCounter = 0;
};

TEST(BluetoothMIDIInterface, initializeBegin) {
    BluetoothMIDI_Interface midi;
    midi.begin();
}

TEST(BluetoothMIDIInterface, notInitialized) {
    BluetoothMIDI_Interface midi;
    // Destructor shouldn't fail if thread wasn't started
}

TEST(BluetoothMIDIInterface, receiveChannelMessage) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0x90, 0x3C, 0x7F};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {
        {0x90, 0x3C, 0x7F},
    };
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveMultipleChannelMessage) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0x90, 0x3C, 0x7F, 0x80, 0x80,
                      0x3D, 0x7E, 0x80, 0xB1, 0x10, 0x40};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {
        {0x90, 0x3C, 0x7F},
        {0x80, 0x3D, 0x7E},
        {0xB1, 0x10, 0x40},
    };
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveMultipleChannelMessageRunningStatus) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0x90, 0x3C, 0x7F, 0x3D,
                      0x7E, 0x80, 0xB1, 0x10, 0x40};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {
        {0x90, 0x3C, 0x7F},
        {0x90, 0x3D, 0x7E},
        {0xB1, 0x10, 0x40},
    };
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface,
     receiveMultipleChannelMessageRunningStatusRealTime) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0x90, 0x3C,
                      0x7F, 0x80, 0xF8, 0x80, // Real Time
                      0x3D, 0x7E,             // Continuation of note on
                      0x80, 0xB1, 0x10, 0x40};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {
        {0x90, 0x3C, 0x7F},
        {0x90, 0x3D, 0x7E},
        {0xB1, 0x10, 0x40},
    };
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveMultipleTwoByteChannelMessage) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0xD0, 0x3C, 0x80, 0xC0,
                      0x3D, 0x80, 0xB1, 0x10, 0x40};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {
        {0xD0, 0x3C, 0x00},
        {0xC0, 0x3D, 0x00},
        {0xB1, 0x10, 0x40},
    };
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveSysEx) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x80, 0x80, 0xF0, 0x01, 0x02, 0x03, 0x04, 0x80, 0xF7};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {0xF0, 0x01, 0x02,
                                                  0x03, 0x04, 0xF7};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 1);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveSysEx2) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x95, 0xED, 0xF0, 0x1, 0x2, 0x3, 0x4, 0xED, 0xF7};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {0xF0, 0x01, 0x02,
                                                  0x03, 0x04, 0xF7};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 1);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveSysExSplitAcrossPackets) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data1[] = {0x81, 0x82, 0xF0, 0x01, 0x02};
    uint8_t data2[] = {0x83, 0x03, 0x04, 0x84, 0xF7};
    midi.parse(data1, sizeof(data1));
    midi.parse(data2, sizeof(data2));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {0xF0, 0x01, 0x02,
                                                  0x03, 0x04, 0xF7};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 1);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, receiveSysExAndRealTime) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x81, 0x82, 0xF0, 0x01, 0x02,
                      0x83, 0xF8, 0x84, // this is a system real time message
                      0x03, 0x04, 0x85, 0xF7};
    midi.parse(data, sizeof(data));
    midi.update();

    std::vector<uint8_t> expectedSysExMessages = {0xF0, 0x01, 0x02,
                                                  0x03, 0x04, 0xF7};
    EXPECT_EQ(cb.sysExMessages, expectedSysExMessages);
    EXPECT_EQ(cb.sysExCounter, 1);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, emptyPacket) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x81}; // Just a header and nothing else
    midi.parse(data, sizeof(data));
    midi.update();

    EXPECT_TRUE(cb.sysExMessages.empty());
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

TEST(BluetoothMIDIInterface, invalidPacket) {
    MockMIDI_Callbacks cb;

    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.setCallbacks(&cb);

    uint8_t data[] = {0x12, 0x13, 0x14}; // First byte is not a header
    midi.parse(data, sizeof(data));
    midi.update();

    EXPECT_TRUE(cb.sysExMessages.empty());
    EXPECT_EQ(cb.sysExCounter, 0);

    std::vector<ChannelMessage> expectedChannelMessages = {};
    EXPECT_EQ(cb.channelMessages, expectedChannelMessages);
}

using namespace ::testing;

static uint16_t timestamp(uint8_t msb, uint8_t lsb) {
    return (uint16_t(msb) << 7) | lsb;
}

TEST(BluetoothMIDIInterface, sendOneNoteMessage) {
    BluetoothMIDI_Interface midi;
    midi.begin();

    std::vector<uint8_t> expected = {0x81, 0x82, 0x92, 0x12, 0x34};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));
    EXPECT_CALL(midi, notifyMIDIBLE(expected));

    midi.sendNoteOn({0x12, CHANNEL_3}, 0x34);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendTwoNoteMessages) {
    BluetoothMIDI_Interface midi;
    midi.begin();

    std::vector<uint8_t> expected = {
        0x81, 0x82, 0x92, 0x12, 0x34, 0x82, 0x99, 0x56, 0x78,
    };
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(2) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));
    EXPECT_CALL(midi, notifyMIDIBLE(expected));

    midi.sendNoteOn({0x12, CHANNEL_3}, 0x34);
    midi.sendNoteOn({0x56, CHANNEL_10}, 0x78);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendTwoNoteMessagesRunningStatus) {
    BluetoothMIDI_Interface midi;
    midi.begin();

    std::vector<uint8_t> expected = {
        0x81, 0x82, 0x92, 0x12, 0x34, 0x56, 0x78,
    };
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(2) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));
    EXPECT_CALL(midi, notifyMIDIBLE(expected));

    midi.sendNoteOn({0x12, CHANNEL_3}, 0x34);
    midi.sendNoteOn({0x56, CHANNEL_3}, 0x78);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendNoteMessageBufferFull) {
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(7 + 3);

    std::vector<uint8_t> expected1 = {0x81, 0x82, 0x85, 0x56, 0x78};
    std::vector<uint8_t> expected2 = {0x81, 0x83, 0x86, 0x66, 0x79};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(2) // For time stamp
        .WillOnce(Return(timestamp(0x01, 0x02)))
        .WillOnce(Return(timestamp(0x01, 0x03)));
    testing::Sequence s;
    EXPECT_CALL(midi, notifyMIDIBLE(expected1)).InSequence(s);
    EXPECT_CALL(midi, notifyMIDIBLE(expected2)).InSequence(s);

    midi.sendNoteOff({0x56, CHANNEL_6}, 0x78);
    midi.sendNoteOff({0x66, CHANNEL_7}, 0x79);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendOneProgramChangeMessage) {
    BluetoothMIDI_Interface midi;
    midi.begin();

    std::vector<uint8_t> expected = {0x81, 0x82, 0xC5, 0x78};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));
    EXPECT_CALL(midi, notifyMIDIBLE(expected));

    midi.sendProgramChange(CHANNEL_6, 0x78);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendProgramChangeMessageBufferFull) {
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(6 + 3);

    std::vector<uint8_t> expected1 = {0x81, 0x82, 0xC5, 0x78};
    std::vector<uint8_t> expected2 = {0x81, 0x83, 0xC6, 0x79};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(2) // For time stamp
        .WillOnce(Return(timestamp(0x01, 0x02)))
        .WillOnce(Return(timestamp(0x01, 0x03)));
    testing::Sequence s;
    EXPECT_CALL(midi, notifyMIDIBLE(expected1)).InSequence(s);
    EXPECT_CALL(midi, notifyMIDIBLE(expected2)).InSequence(s);

    midi.sendProgramChange(CHANNEL_6, 0x78);
    midi.sendProgramChange(CHANNEL_7, 0x79);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendRealTimeMessage) {
    BluetoothMIDI_Interface midi;
    midi.begin();

    std::vector<uint8_t> expected = {0x81, 0x82, 0xF8};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillOnce(Return(timestamp(0x01, 0x02)));
    EXPECT_CALL(midi, notifyMIDIBLE(expected));

    midi.sendRealTime(0xF8);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendRealTimeMessageBufferFull) {
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(5 + 3);

    std::vector<uint8_t> expected1 = {0x81, 0x82, 0xF8, 0x83, 0xF9};
    std::vector<uint8_t> expected2 = {0x81, 0x84, 0xFA};
    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(3) // For time stamp
        .WillOnce(Return(timestamp(0x01, 0x02)))
        .WillOnce(Return(timestamp(0x01, 0x03)))
        .WillOnce(Return(timestamp(0x01, 0x04)));
    testing::Sequence s;
    EXPECT_CALL(midi, notifyMIDIBLE(expected1)).InSequence(s);
    EXPECT_CALL(midi, notifyMIDIBLE(expected2)).InSequence(s);

    midi.sendRealTime(0xF8);
    midi.sendRealTime(0xF9);
    midi.sendRealTime(0xFA);
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendLongSysEx) {
    std::chrono::milliseconds timeout{100};
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(5 + 3);
    midi.setTimeout(timeout);

    std::vector<uint8_t> sysex = {
        0xF0, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0xF7,
    };

    std::vector<uint8_t> expected[] = {
        {
            0x80 | 0x01, // header + timestamp msb
            0x80 | 0x02, //          timestamp lsb
            0xF0,        // SysEx start
            0x10,        // data
            0x11,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x12,        // data
            0x13,        // data
            0x14,        // data
            0x15,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x16,        // data
            0x80 | 0x02, //          timestamp lsb
            0xF7,        // SysEx end
        },
    };

    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));

    InSequence seq;
    EXPECT_CALL(midi, notifyMIDIBLE(expected[0]));
    EXPECT_CALL(midi, notifyMIDIBLE(expected[1]));

    midi.send(SysExMessage(sysex));
    // First two packets should be sent immediately
    Mock::VerifyAndClear(&midi);

    // Third packet is sent after the timeout
    std::this_thread::sleep_for(timeout * 0.9);
    EXPECT_CALL(midi, notifyMIDIBLE(expected[2]));
    std::this_thread::sleep_for(timeout * 0.2);
    Mock::VerifyAndClear(&midi);

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendSysExBufferFullPacket1) {
    std::chrono::milliseconds timeout{100};
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(5 + 3);
    midi.setTimeout(timeout);

    std::vector<uint8_t> sysex = {0xF0, 0xF7};

    std::vector<uint8_t> expected[] = {
        {
            0x80 | 0x01, // header + timestamp msb
            0x80 | 0x02, //          timestamp lsb
            0x91,        // status
            0x14,        // data 1
            0x15,        // data 2
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x80 | 0x03, //          timestamp lsb
            0xF0,        // SysEx start
            0x80 | 0x03, //          timestamp lsb
            0xF7,        // SysEx end
        },
    };

    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(2) // For time stamp
        .WillOnce(Return(timestamp(0x01, 0x02)))
        .WillOnce(Return(timestamp(0x01, 0x03)));

    InSequence seq;
    EXPECT_CALL(midi, notifyMIDIBLE(expected[0]));
    EXPECT_CALL(midi, notifyMIDIBLE(expected[1]));

    midi.sendNoteOn({0x14, CHANNEL_2}, 0x15);
    midi.send(SysExMessage(sysex));
    midi.flush();

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendLongSysExFlush) {
    std::chrono::milliseconds timeout{100};
    BluetoothMIDI_Interface midi;
    midi.begin();
    midi.forceMinMTU(5 + 3);
    midi.setTimeout(timeout);

    std::vector<uint8_t> sysex = {
        0xF0, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0xF7,
    };

    std::vector<uint8_t> expected[] = {
        {
            0x80 | 0x01, // header + timestamp msb
            0x80 | 0x02, //          timestamp lsb
            0xF0,        // SysEx start
            0x10,        // data
            0x11,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x12,        // data
            0x13,        // data
            0x14,        // data
            0x15,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x16,        // data
            0x80 | 0x02, //          timestamp lsb
            0xF7,        // SysEx end
        },
    };

    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));

    InSequence seq;
    EXPECT_CALL(midi, notifyMIDIBLE(expected[0]));
    EXPECT_CALL(midi, notifyMIDIBLE(expected[1]));

    midi.send(SysExMessage(sysex));
    // First two packets should be sent immediately
    Mock::VerifyAndClear(&midi);

    // Third packet is sent after flush
    EXPECT_CALL(midi, notifyMIDIBLE(expected[2]));
    midi.flush();
    Mock::VerifyAndClear(&midi);

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}

TEST(BluetoothMIDIInterface, sendLongSysExFlushDestructor) {
    std::chrono::milliseconds timeout{100};
    auto midi = std::make_unique<BluetoothMIDI_Interface>();
    midi->begin();
    midi->forceMinMTU(5 + 3);
    midi->setTimeout(timeout);

    std::vector<uint8_t> sysex = {
        0xF0, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0xF7,
    };

    std::vector<uint8_t> expected[] = {
        {
            0x80 | 0x01, // header + timestamp msb
            0x80 | 0x02, //          timestamp lsb
            0xF0,        // SysEx start
            0x10,        // data
            0x11,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x12,        // data
            0x13,        // data
            0x14,        // data
            0x15,        // data
        },
        {
            0x80 | 0x01, // header + timestamp msb
            0x16,        // data
            0x80 | 0x02, //          timestamp lsb
            0xF7,        // SysEx end
        },
    };

    EXPECT_CALL(ArduinoMock::getInstance(), millis())
        .Times(1) // For time stamp
        .WillRepeatedly(Return(timestamp(0x01, 0x02)));

    InSequence seq;
    EXPECT_CALL(*midi, notifyMIDIBLE(expected[0]));
    EXPECT_CALL(*midi, notifyMIDIBLE(expected[1]));

    midi->send(SysExMessage(sysex));
    // First two packets should be sent immediately
    Mock::VerifyAndClear(&*midi);

    // Third packet is sent when the MIDI interface is destroyed
    EXPECT_CALL(*midi, notifyMIDIBLE(expected[2]));
    midi.reset();
    Mock::VerifyAndClear(&*midi);

    Mock::VerifyAndClear(&ArduinoMock::getInstance());
}
