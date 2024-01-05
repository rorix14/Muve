/*
	This file controls audio output to the hardware.
	Currently, Windows only
*/
#pragma once

//Might need to uncomment to run on Visual Studio
//#pragma comment(lib, "winmm.lib")

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <Windows.h>
#include <cmath>

template<class T>
class NoiseMaker {
public:
    explicit NoiseMaker(std::wstring outputDevice, unsigned int sampleRate = 44100, unsigned int channels = 1,
                        unsigned int blocks = 8, unsigned int blockSamples = 512) {
        if (!Create(outputDevice, sampleRate, channels, blocks, blockSamples))
            std::cout << "Could not correctly initiate Noise Maker class" << std::endl;
    }

    ~NoiseMaker() {
        Stop();
        Destroy();
    }

    double UserProcess(int channel, double dTime) { return 0.0; }

    const double &GetTime() { return _globalTime; }

    void SetUserFunction(double(*func)(int, double)) { _userFunction = func; }

    static std::vector<std::wstring> Enumerate() {
        unsigned int deviceCount = waveOutGetNumDevs();
        std::vector<std::wstring> devices;
        WAVEOUTCAPS woc;

        for (int n = 0; n < deviceCount; n++)
            if (waveOutGetDevCaps(n, &woc, sizeof(WAVEOUTCAPS)) == S_OK)
                devices.emplace_back(woc.szPname, woc.szPname + wcslen(reinterpret_cast<const wchar_t *>(woc.szPname)));

        return devices;
    }

private:
    double (*_userFunction)(int, double){};

    unsigned int _sampleRate{};
    unsigned int _channels{};
    unsigned int _blockCount{};
    unsigned int _blockSamples{};
    unsigned int _blockCurrent{};

    T *_blockMemory;
    WAVEHDR *_waveHeaders{};
    HWAVEOUT _hwDevice{};

    std::thread _thread;
    bool _ready{};
    std::atomic<unsigned int> _blockFree{};
    std::condition_variable _cvBlockNotZero;
    std::mutex _muxBlockNotZero;

    double _globalTime{};

    bool Create(const std::wstring &outputDevice, unsigned int sampleRate, unsigned int channels,
                unsigned int blocks, unsigned int blockSamples) {
        _ready = true;
        _sampleRate = sampleRate;
        _channels = channels;
        _blockCount = blocks;
        _blockSamples = blockSamples;
        _blockFree = _blockCount;
        _blockCurrent = 0;
        _blockMemory = nullptr;
        _waveHeaders = nullptr;
        _userFunction = nullptr;

        // Validate device
        std::vector<std::wstring> devices = Enumerate();
        auto d = std::find(devices.begin(), devices.end(), outputDevice);
        if (d != devices.end()) {
            unsigned int nDeviceID = std::distance(devices.begin(), d);
            WAVEFORMATEX waveFormat;
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nSamplesPerSec = _sampleRate;
            waveFormat.wBitsPerSample = sizeof(T) * 8;
            waveFormat.nChannels = _channels;
            waveFormat.nBlockAlign = (waveFormat.wBitsPerSample / 8) * waveFormat.nChannels;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            // Open Device if valid
            if (waveOutOpen(&_hwDevice, nDeviceID, &waveFormat, (DWORD_PTR) WaveOutProcWrap, (DWORD_PTR) this,
                            CALLBACK_FUNCTION) != S_OK)
                return Destroy();
        }

        // Allocate Wave Block Memory
        _blockMemory = new T[_blockCount * _blockSamples];
        if (_blockMemory == nullptr)
            return Destroy();
        ZeroMemory(_blockMemory, sizeof(T) * _blockCount * _blockSamples);

        _waveHeaders = new WAVEHDR[_blockCount];
        ZeroMemory(_waveHeaders, sizeof(WAVEHDR) * _blockCount);

        // Link headers to block memory
        for (unsigned int n = 0; n < _blockCount; n++) {
            _waveHeaders[n].dwBufferLength = _blockSamples * sizeof(T);
            _waveHeaders[n].lpData = (LPSTR) (_blockMemory + (n * _blockSamples));
        }

        _thread = std::thread(&NoiseMaker::MainThread, this);

        return true;
    }

    bool Destroy() {
        delete[] _blockMemory;
        delete[] _waveHeaders;
        return false;
    }

    void Stop() {
        _ready = false;
        _thread.join();
    }

    double Clip(double sample, double max) {
        return sample >= 0.0 ? fmin(sample, max) : fmax(sample, -max);
    }


    // Static wrapper for sound card handler
    static void CALLBACK
    WaveOutProcWrap(HWAVEOUT waveOut, UINT msg, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2) {
        if (msg != WOM_DONE) return;

        auto *noiseMakerInstance = reinterpret_cast<NoiseMaker<T> *>(instance);
        noiseMakerInstance->WaveOutProc(msg);
    }

    // Handler for sound card request for more data
    void WaveOutProc(UINT msg) {
        _blockFree++;
        std::unique_lock<std::mutex> lm(_muxBlockNotZero);
        _cvBlockNotZero.notify_one();
    }

    // Main thread. This loop responds to requests from the sound card to fill 'blocks'
    // with audio data. If no requests are available it goes dormant until the sound
    // card is ready for more data. The block is filled by the "user" in some manner
    // and then issued to the sound card.
    void MainThread() {
        _globalTime = 0.0;
        double timeStep = 1.0 / (double) _sampleRate;

        // get maximum integer for a type at run-time
        auto maxSample = (double) pow(2, (sizeof(T) * 8) - 1) - 1;

        while (_ready) {
            // Wait for block to become available
            if (_blockFree == 0) {
                std::unique_lock<std::mutex> lm(_muxBlockNotZero);
                while (_blockFree == 0) // sometimes, Windows signals incorrectly
                    _cvBlockNotZero.wait(lm);
            }

            _blockFree--;

            // Prepare block for processing
            if (_waveHeaders[_blockCurrent].dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(_hwDevice, &_waveHeaders[_blockCurrent], sizeof(WAVEHDR));

            T newSample;
            int currentBlock = _blockCurrent * _blockSamples;

            for (unsigned int n = 0; n < _blockSamples; n += _channels) {
                for (int c = 0; c < _channels; c++) {
                    // User Process
                    if (_userFunction == nullptr)
                        newSample = (T) (Clip(UserProcess(c, _globalTime), 1.0) * maxSample);
                    else
                        newSample = (T) (Clip(_userFunction(c, _globalTime), 1.0) * maxSample);

                    _blockMemory[currentBlock + n + c] = newSample;
                }

                _globalTime = _globalTime + timeStep;
            }

            // Send block to sound device
            waveOutPrepareHeader(_hwDevice, &_waveHeaders[_blockCurrent], sizeof(WAVEHDR));
            waveOutWrite(_hwDevice, &_waveHeaders[_blockCurrent], sizeof(WAVEHDR));
            _blockCurrent++;
            _blockCurrent %= _blockCount;
        }
    }
};
