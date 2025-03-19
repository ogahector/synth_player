

#ifdef TEST_SCANKEYS
void testScanKeys(int state_choice);
#endif
#ifdef TEST_DISPLAYUPDATE
void testDisplayUpdate(int state_choice);
#endif
#ifdef TEST_DECODE
void testDecode();
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
#endif
// void testDecode();
// void testTransmit();