

#ifdef TEST_SCANKEYS
void testScanKeys(int state_choice);
#endif
#ifdef TEST_DISPLAYUPDATE
void testDisplayUpdate(int state_choice);
#endif
#ifdef TEST_DECODE
void testDecode(int state);
#endif
#ifdef TEST_TRANSMIT
void testTransmit();
#endif
#ifdef TEST_SIGGEN
void computeValues();
inline void fillBufferTest(waveform_t wave, volatile uint8_t buffer[], uint32_t size, int volume);
void testSigGen(int wave);
#endif
#ifdef TEST_RECORD
void fillPLayback();
void testRecord(int state);
void releaseAllNotesTest(uint8_t track, QueueHandle_t queue, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes);
void removeNoteTest(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes);
void addNoteTest(uint8_t track, uint8_t octave, uint8_t key, std::vector<std::vector<std::pair<uint8_t,uint8_t>>> &activeNotes);
#endif
// void testDecode();
// void testTransmit();