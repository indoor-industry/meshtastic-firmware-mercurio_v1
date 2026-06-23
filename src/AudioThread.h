#pragma once
#include "PowerFSM.h"
#include "concurrency/OSThread.h"
#include "configuration.h"
#include "main.h"
#include "sleep.h"
#include <memory>

#ifdef HAS_I2S
#include <AudioFileSourcePROGMEM.h>
#include <AudioGeneratorRTTTL.h>
#include <AudioOutputI2S.h>
#include <ESP8266SAM.h>

#ifdef USE_XL9555
#include "ExtensionIOXL9555.hpp"
extern ExtensionIOXL9555 io;
#endif

#define AUDIO_THREAD_INTERVAL_MS 100

class AudioThread : public concurrency::OSThread
{
  public:
    AudioThread() : OSThread("Audio") { initOutput(); }

    void beginRttl(const void *data, uint32_t len)
    {
#ifdef T_LORA_PAGER
        io.digitalWrite(EXPANDS_AMP_EN, HIGH);
#endif
#ifdef MERCURIO_V1
        mercurio_setAudioEnable(true);
#endif
        setCPUFast(true);
        rtttlFile = std::unique_ptr<AudioFileSourcePROGMEM>(new AudioFileSourcePROGMEM(data, len));
        i2sRtttl = std::unique_ptr<AudioGeneratorRTTTL>(new AudioGeneratorRTTTL());
        i2sRtttl->begin(rtttlFile.get(), audioOut.get());
    }

    // Also handles actually playing the RTTTL, needs to be called in loop
    bool isPlaying()
    {
        if (i2sRtttl != nullptr) {
            return i2sRtttl->isRunning() && i2sRtttl->loop();
        }
        return false;
    }

    void stop()
    {
        if (i2sRtttl != nullptr) {
            i2sRtttl->stop();
            i2sRtttl = nullptr;
        }

        rtttlFile = nullptr;

        setCPUFast(false);
#ifdef T_LORA_PAGER
        io.digitalWrite(EXPANDS_AMP_EN, LOW);
#endif
#ifdef MERCURIO_V1
        mercurio_setAudioEnable(false);
#endif
    }

    void readAloud(const char *text)
    {
        if (i2sRtttl != nullptr) {
            i2sRtttl->stop();
            i2sRtttl = nullptr;
        }

#ifdef T_LORA_PAGER
        io.digitalWrite(EXPANDS_AMP_EN, HIGH);
#endif
#ifdef MERCURIO_V1
        mercurio_setAudioEnable(true);
#endif
        auto sam = std::unique_ptr<ESP8266SAM>(new ESP8266SAM);
        sam->Say(audioOut.get(), text);
        setCPUFast(false);
#ifdef T_LORA_PAGER
        io.digitalWrite(EXPANDS_AMP_EN, LOW);
#endif
#ifdef MERCURIO_V1
        mercurio_setAudioEnable(false);
#endif
    }

  protected:
    int32_t runOnce() override
    {
        canSleep = true; // Assume we should not keep the board awake

        // if (i2sRtttl != nullptr && i2sRtttl->isRunning()) {
        //     i2sRtttl->loop();
        // }
#ifdef MERCURIO_V1
        // SW1/SW2 aren't on free GPIOs (behind the MCP23017), so they're
        // polled here rather than via a GPIO interrupt. Held-down repeats
        // naturally at this thread's AUDIO_THREAD_INTERVAL_MS cadence.
        uint8_t buttons = mercurio_readButtons();
        if (buttons & 0x01)
            adjustGain(0.05f);
        if (buttons & 0x02)
            adjustGain(-0.05f);
#endif
        return AUDIO_THREAD_INTERVAL_MS;
    }

  private:
    void initOutput()
    {
        audioOut = std::unique_ptr<AudioOutputI2S>(new AudioOutputI2S(1, AudioOutputI2S::EXTERNAL_I2S));
        audioOut->SetPinout(DAC_I2S_BCK, DAC_I2S_WS, DAC_I2S_DOUT, DAC_I2S_MCLK);
#ifdef MERCURIO_V1
        // This board's MAX98357A needs the no-delay STAND_MSB I2S format,
        // not the default STAND_I2S (1-BCLK delay) - see test_firmware/src/audio_i2s.cpp.
        audioOut->SetLsbJustified(true);
        // Mono amp: without this, the right I2S slot carries whatever the
        // (single-channel) TTS/RTTTL source leaves there, which the amp may
        // mix into the output - audible as distortion/bad quality even
        // though audio otherwise plays.
        audioOut->SetOutputModeMono(true);
#endif
        audioOut->SetGain(gain);
    };

#ifdef MERCURIO_V1
    void adjustGain(float delta)
    {
        gain = constrain(gain + delta, 0.0f, 1.0f); // 0.0 = mute, holding SW2 down all the way
        if (audioOut != nullptr)
            audioOut->SetGain(gain);
    }
#endif

    float gain = 0.2f;
    std::unique_ptr<AudioGeneratorRTTTL> i2sRtttl = nullptr;
    std::unique_ptr<AudioOutputI2S> audioOut = nullptr;

    std::unique_ptr<AudioFileSourcePROGMEM> rtttlFile = nullptr;
};

#endif
